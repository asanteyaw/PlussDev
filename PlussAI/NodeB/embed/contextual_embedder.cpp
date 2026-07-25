#include "pluss/ai/nodeb/embed.h"

#include "../io/io_detail.h"
#include "../tasks/deep/sequence_heads.h"
#include "pluss/ai/ice/optim.h"
#include "pluss/ai/nodeb/runtime.h"
#include "pluss/nn.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <numeric>
#include <span>
#include <sstream>

namespace pluss::ai::nodeb::embed {

struct ContextualEmbedder::Impl {
  runtime::Indexer indexer{};
  std::shared_ptr<runtime::EmbeddingTable> embedding{};
  std::shared_ptr<runtime::TransformerTextEncoder> encoder{};
  // The causal-LM output head (encoder output -> next-token vocab logits).
  // Only needed for training and for `generate()`'s decode loop — `encode`/
  // `embed` never touch it — but it's kept (rather than discarded after
  // `learn()`, as it used to be) so both save()/load() and generate() have
  // something to persist/use.
  Series output_weight = Series::undefined();
  Series output_bias = Series::undefined();
  bool trained = false;
};

ContextualEmbedder::ContextualEmbedder(ContextualEmbedOptions options)
    : impl_(std::make_shared<Impl>()), options_(options) {}

bool ContextualEmbedder::ready() const noexcept {
  return impl_ && impl_->trained;
}

std::int64_t ContextualEmbedder::width() const noexcept {
  return options_.width;
}

nodej::Status ContextualEmbedder::learn(const std::vector<Doc>& docs) {
  PLUSS_CALL_GUARD();
  if (docs.empty()) {
    return nodej::Status::invalid_argument(
        "nodeb::embed::ContextualEmbedder::learn: expected at least one document.");
  }
  if (options_.width <= 0 || options_.num_layers <= 0 || options_.num_heads <= 0 ||
      options_.feedforward_width <= 0 || options_.epochs <= 0 || options_.batch_size <= 0) {
    return nodej::Status::invalid_argument(
        "nodeb::embed::ContextualEmbedder::learn: width, num_layers, num_heads, "
        "feedforward_width, epochs and batch_size must all be positive.");
  }
  if (options_.width % options_.num_heads != 0) {
    return nodej::Status::invalid_argument(
        "nodeb::embed::ContextualEmbedder::learn: width must be a multiple of num_heads.");
  }

  runtime::SequenceOptions seq_options{};
  seq_options.use_lemmas = options_.use_lemmas;
  seq_options.skip_punct = options_.skip_punct;
  seq_options.lowercase = options_.lowercase;
  seq_options.add_bos = true;
  seq_options.add_eos = true;
  seq_options.batch_first = true;

  runtime::Indexer indexer(seq_options);
  const auto indexer_status = indexer.learn(docs);
  if (!indexer_status.ok()) {
    return indexer_status;
  }

  const auto batch_or = indexer.batch(docs);
  if (!batch_or.ok()) {
    return batch_or.status();
  }
  const auto& full_batch = batch_or.value();
  if (full_batch.empty() || full_batch.max_length < 2) {
    return nodej::Status::precondition(
        "nodeb::embed::ContextualEmbedder::learn: documents are too short to train a "
        "next-token objective (need at least 2 tokens once framed with <bos>/<eos>).");
  }

  const auto vocab_size = static_cast<std::int64_t>(indexer.vocabulary().size());
  const auto width = options_.width;
  const auto cfg = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
  const auto seq_len = full_batch.max_length - 1; // after the next-token shift.

  auto embedding = std::make_shared<runtime::EmbeddingTable>(runtime::EmbeddingOptions{
      .width = width,
      .use_positions = true,
      .learned_positions = true,
      // Generous, fixed headroom: inference-time documents longer than this
      // (after <bos>/<eos> framing) are outside what this model supports.
      .max_positions = std::max<std::int64_t>(full_batch.max_length + 1, 512),
      .cfg = cfg,
      .seed = options_.seed,
  });
  embedding->reset(vocab_size, indexer.pad_id());
  embedding->weights().tracks_slope_(true);
  embedding->position_weights().tracks_slope_(true);

  auto encoder = std::make_shared<runtime::TransformerTextEncoder>(pluss::nn::TransformerEncoderOptions{
      .d_model = width,
      .nhead = options_.num_heads,
      .num_layers = options_.num_layers,
      .dim_feedforward = options_.feedforward_width,
      .dropout = 0.1,
      .batch_first = true,
      .cfg = cfg,
      .seed = options_.seed + 1,
  });
  encoder->model()->train(true);

  const double scale = 0.5 / static_cast<double>(width);
  Series output_weight = pluss::rand({width, vocab_size}, cfg, options_.seed + 2).sub(0.5).mul(2.0 * scale);
  Series output_bias = pluss::zeros({vocab_size}, cfg);
  output_weight.tracks_slope_(true);
  output_bias.tracks_slope_(true);

  std::vector<Series*> parameters = encoder->model()->parameters();
  parameters.push_back(&const_cast<Series&>(embedding->weights()));
  parameters.push_back(&const_cast<Series&>(embedding->position_weights()));
  parameters.push_back(&output_weight);
  parameters.push_back(&output_bias);

  // Same shape mismatch as embed::Embedder/detail::train_token_classifier:
  // architecture is sized off a data-discovered vocab and the input is a
  // structured SequenceBatch, not a flat Series, so this doesn't fit
  // ice::Model — but ice::make_optimizer adds gradient clipping and
  // non-finite diagnostics this loop previously had none of.
  pluss::ai::ice::TrainOptions optimizer_options{};
  optimizer_options.optimizer = pluss::ai::ice::OptimizerKind::Adam;
  optimizer_options.learning_rate = options_.learning_rate;
  auto optimizer = pluss::ai::ice::make_optimizer(parameters, optimizer_options);

  // Next-token shift: input = tokens[0 .. T-2], target = tokens[1 .. T-1].
  runtime::SequenceBatch input_batch{};
  input_batch.token_ids = full_batch.token_ids.narrow_copy(1, 0, seq_len);
  input_batch.position_ids = full_batch.position_ids.narrow_copy(1, 0, seq_len);
  input_batch.valid_mask = full_batch.valid_mask.narrow_copy(1, 0, seq_len);
  input_batch.key_padding_mask = full_batch.key_padding_mask.narrow_copy(1, 0, seq_len);
  input_batch.batch_size = full_batch.batch_size;
  input_batch.max_length = seq_len;
  input_batch.batch_first = true;

  const Series target_ids = full_batch.token_ids.narrow_copy(1, 1, seq_len).to(pluss::pCPU, ScalarType::Int64);

  pluss::nn::losses::CrossEntropyOptions ce_options{};
  ce_options.use_ignore_index = true;
  ce_options.ignore_index = indexer.pad_id();

  double previous_loss = std::numeric_limits<double>::infinity();
  try {
    for (std::int64_t epoch = 0; epoch < options_.epochs; ++epoch) {
      const Series embedded = embedding->forward(input_batch);                         // [N, seq_len, width]
      const Series encoded = encoder->forward(embedded, input_batch, /*causal=*/true);  // [N, seq_len, width]
      const Series flat_encoded = encoded.reshape({full_batch.batch_size * seq_len, width});
      const Series logits = flat_encoded.matmul(output_weight)
                                .add(output_bias)
                                .reshape({full_batch.batch_size, seq_len, vocab_size});
      const Series loss = pluss::nn::losses::CrossEntropy(logits, target_ids, ce_options);

      optimizer->clear();
      loss.reverse();
      optimizer->step();

      const double loss_value = loss.to(pluss::pCPU, ScalarType::Float64).item<double>();
      if (std::abs(previous_loss - loss_value) < 1e-6) {
        previous_loss = loss_value;
        break;
      }
      previous_loss = loss_value;
    }
  } catch (const std::exception& e) {
    return nodej::Status::numerical_failure(
        std::string("nodeb::embed::ContextualEmbedder::learn: training became numerically unstable: ") + e.what());
  }

  encoder->model()->eval();
  embedding->weights().tracks_slope_(false);
  embedding->position_weights().tracks_slope_(false);
  output_weight.tracks_slope_(false);
  output_bias.tracks_slope_(false);

  auto impl = std::make_shared<Impl>();
  impl->indexer = std::move(indexer);
  impl->embedding = std::move(embedding);
  impl->encoder = std::move(encoder);
  impl->output_weight = std::move(output_weight);
  impl->output_bias = std::move(output_bias);
  impl->trained = true;
  impl_ = std::move(impl);
  return nodej::Status::success();
}

nodej::StatusOr<std::vector<std::vector<double>>> ContextualEmbedder::encode(const Doc& doc) const {
  using Result = std::vector<std::vector<double>>;
  if (!ready()) {
    return nodej::StatusOr<Result>(
        nodej::Status::precondition("nodeb::embed::ContextualEmbedder::encode: model has not been trained."));
  }

  const auto batch_or = impl_->indexer.batch(std::vector<Doc>{doc});
  if (!batch_or.ok()) {
    return nodej::StatusOr<Result>(batch_or.status());
  }
  const auto& batch = batch_or.value();
  if (batch.empty() || batch.max_length <= 2) {
    return nodej::StatusOr<Result>(nodej::Status::precondition(
        "nodeb::embed::ContextualEmbedder::encode: document produced no content tokens."));
  }

  const Series embedded = impl_->embedding->forward(batch);
  // Match training exactly: the model's attention weights were optimized
  // under causal masking, so inference must use it too.
  const Series encoded = impl_->encoder->forward(embedded, batch, /*causal=*/true);

  // `batch` holds exactly one document, so its declared length equals
  // max_length (no cross-document padding); positions [1, length-2] are the
  // real content tokens, excluding the synthetic <bos>/<eos> framing.
  const auto content_len = batch.max_length - 2;
  const Series content = encoded.narrow_copy(1, 1, content_len).reshape({content_len, options_.width});
  const auto flat =
      content.to(pluss::pCPU, ScalarType::Float64).reshape({content_len * options_.width}).to_vec<double>();

  Result out(static_cast<std::size_t>(content_len));
  for (std::int64_t i = 0; i < content_len; ++i) {
    const auto offset = static_cast<std::size_t>(i * options_.width);
    out[static_cast<std::size_t>(i)].assign(
        flat.begin() + static_cast<std::ptrdiff_t>(offset),
        flat.begin() + static_cast<std::ptrdiff_t>(offset + static_cast<std::size_t>(options_.width)));
  }
  return nodej::StatusOr<Result>(std::move(out));
}

nodej::StatusOr<std::vector<double>> ContextualEmbedder::embed(const Doc& doc) const {
  const auto encoded_or = encode(doc);
  if (!encoded_or.ok()) {
    return nodej::StatusOr<std::vector<double>>(encoded_or.status());
  }
  const auto& rows = encoded_or.value();
  std::vector<double> pooled(static_cast<std::size_t>(options_.width), 0.0);
  for (const auto& row : rows) {
    for (std::size_t i = 0; i < row.size(); ++i) {
      pooled[i] += row[i];
    }
  }
  for (auto& value : pooled) {
    value /= static_cast<double>(rows.size());
  }
  return nodej::StatusOr<std::vector<double>>(std::move(pooled));
}

namespace {

// Builds a fresh, unpadded, single-sequence SequenceBatch (batch_size=1) for
// `sequence` -- generation grows one sequence at a time and re-encodes it
// from scratch every step (no KV-cache), so there is never any padding to
// mask: every position is real and valid.
[[nodiscard]] runtime::SequenceBatch make_single_sequence_batch(const std::vector<std::int64_t>& sequence) {
  const auto length = static_cast<std::int64_t>(sequence.size());
  const Config idx_cfg = Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
  const Config bool_cfg = Config{}.with_dtype(ScalarType::Bool).with_platform(pluss::pCPU);

  std::vector<std::int64_t> position_ids(static_cast<std::size_t>(length));
  std::iota(position_ids.begin(), position_ids.end(), std::int64_t{0});
  const std::vector<bool> valid_mask(static_cast<std::size_t>(length), true);
  const std::vector<bool> key_padding_mask(static_cast<std::size_t>(length), false);

  runtime::SequenceBatch batch{};
  batch.token_ids = pluss::make_series(sequence, std::vector<std::int64_t>{1, length}, idx_cfg);
  batch.position_ids = pluss::make_series(position_ids, std::vector<std::int64_t>{1, length}, idx_cfg);
  batch.valid_mask = pluss::make_series(valid_mask, std::vector<std::int64_t>{1, length}, bool_cfg);
  batch.key_padding_mask = pluss::make_series(key_padding_mask, std::vector<std::int64_t>{1, length}, bool_cfg);
  batch.lengths = pluss::make_series(std::vector<std::int64_t>{length}, std::vector<std::int64_t>{1}, idx_cfg);
  batch.batch_size = 1;
  batch.max_length = length;
  batch.batch_first = true;
  return batch;
}

// Softmax on the host, with an optional top-k restriction (renormalized
// over the kept mass). `logits` and the return value are both length
// `vocab_size`.
[[nodiscard]] std::vector<double> next_token_probs(const std::vector<double>& logits, std::int64_t top_k) {
  const double max_logit = *std::max_element(logits.begin(), logits.end());
  std::vector<double> probs(logits.size());
  double denom = 0.0;
  for (std::size_t i = 0; i < logits.size(); ++i) {
    probs[i] = std::exp(logits[i] - max_logit);
    denom += probs[i];
  }
  for (double& p : probs) p /= denom;

  if (top_k <= 0 || top_k >= static_cast<std::int64_t>(probs.size())) {
    return probs;
  }
  std::vector<std::size_t> order(probs.size());
  std::iota(order.begin(), order.end(), std::size_t{0});
  std::partial_sort(order.begin(), order.begin() + top_k, order.end(),
                    [&](std::size_t a, std::size_t b) { return probs[a] > probs[b]; });
  std::vector<double> filtered(probs.size(), 0.0);
  double kept_mass = 0.0;
  for (std::int64_t k = 0; k < top_k; ++k) {
    const auto idx = order[static_cast<std::size_t>(k)];
    filtered[idx] = probs[idx];
    kept_mass += probs[idx];
  }
  for (double& p : filtered) p /= kept_mass;
  return filtered;
}

} // namespace

nodej::StatusOr<std::vector<std::string>> ContextualEmbedder::generate(const Doc& prompt,
                                                                        const GenerateOptions& options) const {
  using Result = std::vector<std::string>;
  if (!ready()) {
    return nodej::StatusOr<Result>(
        nodej::Status::precondition("nodeb::embed::ContextualEmbedder::generate: model has not been trained."));
  }
  if (options.max_new_tokens <= 0) {
    return nodej::StatusOr<Result>(nodej::Status::invalid_argument(
        "nodeb::embed::ContextualEmbedder::generate: max_new_tokens must be positive."));
  }
  if (options.temperature <= 0.0) {
    return nodej::StatusOr<Result>(
        nodej::Status::invalid_argument("nodeb::embed::ContextualEmbedder::generate: temperature must be positive."));
  }

  const auto batch_or = impl_->indexer.batch(std::vector<Doc>{prompt});
  if (!batch_or.ok()) {
    return nodej::StatusOr<Result>(batch_or.status());
  }
  const auto& prompt_batch = batch_or.value();
  if (prompt_batch.empty() || prompt_batch.max_length < 2) {
    return nodej::StatusOr<Result>(nodej::Status::precondition(
        "nodeb::embed::ContextualEmbedder::generate: failed to frame the prompt with <bos>/<eos>."));
  }

  // `learn()` always frames documents with <bos> ... <eos>; drop the
  // trailing <eos> to reopen the sequence for continued generation. What
  // remains (at minimum, just <bos>) is always a valid start.
  const auto prompt_length = prompt_batch.max_length;
  auto sequence = prompt_batch.token_ids.to(pluss::pCPU, ScalarType::Int64)
                      .reshape({prompt_length})
                      .to_vec<std::int64_t>();
  sequence.pop_back();

  const auto width = options_.width;
  const auto vocab_size = static_cast<std::int64_t>(impl_->embedding->vocab_size());
  const auto max_positions = impl_->embedding->options().max_positions;
  const auto eos_id = impl_->indexer.eos_id();
  const auto& vocabulary = impl_->indexer.vocabulary();

  Result generated{};
  std::uint64_t sample_seed = options.seed;
  for (std::int64_t step = 0; step < options.max_new_tokens; ++step) {
    const auto length = static_cast<std::int64_t>(sequence.size());
    if (length >= max_positions) break; // out of position-embedding headroom.

    const auto step_batch = make_single_sequence_batch(sequence);
    const Series embedded = impl_->embedding->forward(step_batch);
    // Same convention as encode()/training: attention was optimized under
    // causal masking, so decoding must use it too.
    const Series encoded = impl_->encoder->forward(embedded, step_batch, /*causal=*/true);
    const Series last = encoded.narrow_copy(1, length - 1, 1).reshape({1, width});
    Series logits = last.matmul(impl_->output_weight).add(impl_->output_bias).reshape({vocab_size});
    if (options.temperature != 1.0) {
      logits = logits.div(options.temperature);
    }

    std::int64_t next_id = 0;
    if (options.greedy) {
      next_id = logits.argmax(0).to(pluss::pCPU, ScalarType::Int64).item<std::int64_t>();
    } else {
      const auto flat_logits = logits.to(pluss::pCPU, ScalarType::Float64).reshape({vocab_size}).to_vec<double>();
      const auto probs = next_token_probs(flat_logits, options.top_k);
      const Series sampled = pluss::multinomial(std::span<const double>(probs), 1, /*replacement=*/true, sample_seed);
      ++sample_seed;
      next_id = sampled.to(pluss::pCPU, ScalarType::Int64).item<std::int64_t>();
    }

    if (next_id == eos_id) break;

    sequence.push_back(next_id);
    if (next_id >= 0 && static_cast<std::size_t>(next_id) < vocabulary.size()) {
      generated.push_back(vocabulary[static_cast<std::size_t>(next_id)]);
    }
  }

  return nodej::StatusOr<Result>(std::move(generated));
}

namespace {

constexpr std::string_view kMagicLine = "pluss_nodeb_contextual_embedder";
constexpr std::int64_t kCurrentSchema = 1;

[[nodiscard]] Config contextual_float_cfg() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

} // namespace

void ContextualEmbedder::save(std::string_view filepath) const {
  PLUSS_CALL_GUARD();
  if (filepath.empty()) {
    ::pluss::ai::nodeb::detail::fail("nodeb::embed::ContextualEmbedder::save: output path cannot be empty.");
  }
  if (!ready()) {
    ::pluss::ai::nodeb::detail::fail("nodeb::embed::ContextualEmbedder::save: model has not been trained.");
  }
  std::ofstream out{std::string(filepath)};
  if (!out.good()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::embed::ContextualEmbedder::save: failed to open '" + std::string(filepath) + "'.");
  }

  std::ostringstream body{};

  const auto indexer_lines = impl_->indexer.dump_state_lines();
  body << "indexer_line_count=" << indexer_lines.size() << "\n";
  for (std::size_t i = 0; i < indexer_lines.size(); ++i) {
    body << "indexer_" << i << "=" << indexer_lines[i] << "\n";
  }

  const auto embedding_lines = impl_->embedding->dump_state_lines();
  body << "embedding_line_count=" << embedding_lines.size() << "\n";
  for (std::size_t i = 0; i < embedding_lines.size(); ++i) {
    body << "embedding_" << i << "=" << embedding_lines[i] << "\n";
  }

  const auto output_weight_flat =
      impl_->output_weight.to(pluss::pCPU, ScalarType::Float64).reshape({impl_->output_weight.numel()}).to_vec<double>();
  const auto output_bias_flat =
      impl_->output_bias.to(pluss::pCPU, ScalarType::Float64).reshape({impl_->output_bias.numel()}).to_vec<double>();
  body << "output_weight=" << detail::join_doubles_line(output_weight_flat) << "\n";
  body << "output_bias=" << detail::join_doubles_line(output_bias_flat) << "\n";

  const auto params = impl_->encoder->model()->parameters();
  body << "encoder_param_count=" << params.size() << "\n";
  for (std::size_t i = 0; i < params.size(); ++i) {
    const auto shape_view = params[i]->sizes();
    const std::vector<double> shape_as_double(shape_view.begin(), shape_view.end());
    const auto flat = params[i]->to(pluss::pCPU, ScalarType::Float64).reshape({params[i]->numel()}).to_vec<double>();
    body << "encoder_param_" << i << "_shape=" << detail::join_doubles_line(shape_as_double) << "\n";
    body << "encoder_param_" << i << "=" << detail::join_doubles_line(flat) << "\n";
  }
  const auto body_text = body.str();

  out << kMagicLine << "\n";
  out << "schema=" << kCurrentSchema << "\n";
  out << "width=" << options_.width << "\n";
  out << "num_layers=" << options_.num_layers << "\n";
  out << "num_heads=" << options_.num_heads << "\n";
  out << "feedforward_width=" << options_.feedforward_width << "\n";
  out << "checksum=" << io::detail::to_hex(io::detail::fnv1a64(body_text)) << "\n";
  out << body_text;

  if (!out.good()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::embed::ContextualEmbedder::save: failed while writing '" + std::string(filepath) + "'.");
  }
}

ContextualEmbedder ContextualEmbedder::load(std::string_view filepath) {
  PLUSS_CALL_GUARD();
  if (filepath.empty()) {
    ::pluss::ai::nodeb::detail::fail("nodeb::embed::ContextualEmbedder::load: input path cannot be empty.");
  }
  std::ifstream in{std::string(filepath)};
  if (!in.good()) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::embed::ContextualEmbedder::load: failed to open '" + std::string(filepath) + "'.");
  }

  std::string line{};
  if (!std::getline(in, line) || line != kMagicLine) {
    ::pluss::ai::nodeb::detail::fail("nodeb::embed::ContextualEmbedder::load: invalid contextual embedder header.");
  }

  std::int64_t schema = -1;
  std::int64_t width = -1;
  std::int64_t num_layers = -1;
  std::int64_t num_heads = -1;
  std::int64_t feedforward_width = -1;
  std::uint64_t expected_checksum = 0;
  bool have_checksum = false;
  while (std::getline(in, line)) {
    if (line.rfind("schema=", 0) == 0) {
      schema = io::detail::parse_i64(std::string_view(line).substr(7));
      continue;
    }
    if (line.rfind("width=", 0) == 0) {
      width = io::detail::parse_i64(std::string_view(line).substr(6));
      continue;
    }
    if (line.rfind("num_layers=", 0) == 0) {
      num_layers = io::detail::parse_i64(std::string_view(line).substr(11));
      continue;
    }
    if (line.rfind("num_heads=", 0) == 0) {
      num_heads = io::detail::parse_i64(std::string_view(line).substr(10));
      continue;
    }
    if (line.rfind("feedforward_width=", 0) == 0) {
      feedforward_width = io::detail::parse_i64(std::string_view(line).substr(18));
      continue;
    }
    if (line.rfind("checksum=", 0) == 0) {
      expected_checksum = io::detail::parse_hex_u64(std::string_view(line).substr(9));
      have_checksum = true;
      break; // checksum is always the last header line; the body follows immediately.
    }
    ::pluss::ai::nodeb::detail::fail("nodeb::embed::ContextualEmbedder::load: unrecognized header line '" + line + "'.");
  }
  if (schema != kCurrentSchema || width <= 0 || num_layers <= 0 || num_heads <= 0 || feedforward_width <= 0 ||
      !have_checksum) {
    ::pluss::ai::nodeb::detail::fail("nodeb::embed::ContextualEmbedder::load: invalid or unsupported header.");
  }

  std::ostringstream body_buffer{};
  body_buffer << in.rdbuf();
  const auto body_text = body_buffer.str();
  if (io::detail::fnv1a64(body_text) != expected_checksum) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::embed::ContextualEmbedder::load: checksum mismatch, artifact is truncated or corrupted.");
  }

  std::vector<std::string> body_lines{};
  {
    std::istringstream body_stream(body_text);
    std::string body_line{};
    while (std::getline(body_stream, body_line)) body_lines.push_back(body_line);
  }

  std::int64_t indexer_line_count = -1;
  std::int64_t embedding_line_count = -1;
  std::int64_t encoder_param_count = -1;
  std::vector<double> output_weight_flat{};
  std::vector<double> output_bias_flat{};
  for (const auto& body_line : body_lines) {
    const auto eq = body_line.find('=');
    if (eq == std::string::npos) continue;
    const std::string_view key(body_line.data(), eq);
    const std::string_view value(body_line.data() + eq + 1, body_line.size() - eq - 1);
    if (key == "indexer_line_count") { indexer_line_count = io::detail::parse_i64(value); continue; }
    if (key == "embedding_line_count") { embedding_line_count = io::detail::parse_i64(value); continue; }
    if (key == "encoder_param_count") { encoder_param_count = io::detail::parse_i64(value); continue; }
    if (key == "output_weight") { output_weight_flat = detail::parse_doubles_line(value); continue; }
    if (key == "output_bias") { output_bias_flat = detail::parse_doubles_line(value); continue; }
  }
  if (indexer_line_count < 0 || embedding_line_count < 0 || encoder_param_count < 0) {
    ::pluss::ai::nodeb::detail::fail("nodeb::embed::ContextualEmbedder::load: malformed contextual embedder state.");
  }

  const auto collect_indexed_lines = [&](const std::string& prefix, std::int64_t count) {
    std::vector<std::string> collected(static_cast<std::size_t>(std::max<std::int64_t>(0, count)));
    for (const auto& body_line : body_lines) {
      if (body_line.rfind(prefix, 0) != 0) continue;
      const std::string_view rest(body_line.data() + prefix.size(), body_line.size() - prefix.size());
      std::size_t digits = 0;
      while (digits < rest.size() && rest[digits] >= '0' && rest[digits] <= '9') ++digits;
      if (digits == 0 || digits >= rest.size() || rest[digits] != '=') continue;
      const auto idx = static_cast<std::size_t>(std::stoll(std::string(rest.substr(0, digits))));
      if (idx >= collected.size()) continue;
      collected[idx] = std::string(rest.substr(digits + 1));
    }
    return collected;
  };

  const auto indexer_lines_raw = collect_indexed_lines("indexer_", indexer_line_count);
  const std::vector<std::string_view> indexer_views(indexer_lines_raw.begin(), indexer_lines_raw.end());
  runtime::Indexer indexer{};
  const auto indexer_status = indexer.load_state_lines(pluss::msharp::RangeView::Of<std::string_view>(indexer_views));
  if (!indexer_status.ok()) {
    ::pluss::ai::nodeb::detail::fail("nodeb::embed::ContextualEmbedder::load: " + indexer_status.message);
  }

  const auto embedding_lines_raw = collect_indexed_lines("embedding_", embedding_line_count);
  const std::vector<std::string_view> embedding_views(embedding_lines_raw.begin(), embedding_lines_raw.end());
  auto embedding = std::make_shared<runtime::EmbeddingTable>();
  const auto embedding_status =
      embedding->load_state_lines(pluss::msharp::RangeView::Of<std::string_view>(embedding_views));
  if (!embedding_status.ok()) {
    ::pluss::ai::nodeb::detail::fail("nodeb::embed::ContextualEmbedder::load: " + embedding_status.message);
  }

  const auto cfg = contextual_float_cfg();
  auto encoder = std::make_shared<runtime::TransformerTextEncoder>(pluss::nn::TransformerEncoderOptions{
      .d_model = width,
      .nhead = num_heads,
      .num_layers = num_layers,
      .dim_feedforward = feedforward_width,
      .dropout = 0.1,
      .batch_first = true,
      .cfg = cfg,
      .seed = 0,
  });
  encoder->model()->eval();

  auto encoder_params = encoder->model()->parameters();
  if (static_cast<std::int64_t>(encoder_params.size()) != encoder_param_count) {
    ::pluss::ai::nodeb::detail::fail(
        "nodeb::embed::ContextualEmbedder::load: encoder parameter count mismatch "
        "(architecture options changed since save?).");
  }

  std::vector<std::string> shape_lines(static_cast<std::size_t>(encoder_param_count));
  std::vector<std::string> value_lines(static_cast<std::size_t>(encoder_param_count));
  const std::string param_prefix = "encoder_param_";
  for (const auto& body_line : body_lines) {
    if (body_line.rfind(param_prefix, 0) != 0) continue;
    const std::string_view rest(body_line.data() + param_prefix.size(), body_line.size() - param_prefix.size());
    std::size_t digits = 0;
    while (digits < rest.size() && rest[digits] >= '0' && rest[digits] <= '9') ++digits;
    if (digits == 0) continue;
    const auto idx = static_cast<std::size_t>(std::stoll(std::string(rest.substr(0, digits))));
    if (idx >= shape_lines.size()) continue;
    const std::string_view tail = rest.substr(digits);
    if (tail.rfind("_shape=", 0) == 0) {
      shape_lines[idx] = std::string(tail.substr(7));
    } else if (tail.rfind("=", 0) == 0) {
      value_lines[idx] = std::string(tail.substr(1));
    }
  }
  for (std::int64_t i = 0; i < encoder_param_count; ++i) {
    const auto idx = static_cast<std::size_t>(i);
    const auto shape_doubles = detail::parse_doubles_line(shape_lines[idx]);
    const std::vector<std::int64_t> shape(shape_doubles.begin(), shape_doubles.end());
    auto values = detail::parse_doubles_line(value_lines[idx]);
    *encoder_params[idx] = pluss::make_series(values, shape, cfg).to(cfg.platform, cfg.dtype);
  }

  if (output_weight_flat.empty() || output_bias_flat.empty()) {
    ::pluss::ai::nodeb::detail::fail("nodeb::embed::ContextualEmbedder::load: missing output head state.");
  }
  const auto vocab_size = static_cast<std::int64_t>(embedding->vocab_size());
  Series output_weight = pluss::make_series(output_weight_flat, std::vector<std::int64_t>{width, vocab_size}, cfg)
                             .to(cfg.platform, cfg.dtype);
  Series output_bias =
      pluss::make_series(output_bias_flat, std::vector<std::int64_t>{vocab_size}, cfg).to(cfg.platform, cfg.dtype);

  ContextualEmbedOptions options{};
  options.width = width;
  options.num_layers = num_layers;
  options.num_heads = num_heads;
  options.feedforward_width = feedforward_width;

  ContextualEmbedder result(options);
  auto impl = std::make_shared<Impl>();
  impl->indexer = std::move(indexer);
  impl->embedding = std::move(embedding);
  impl->encoder = std::move(encoder);
  impl->output_weight = std::move(output_weight);
  impl->output_bias = std::move(output_bias);
  impl->trained = true;
  result.impl_ = std::move(impl);
  return result;
}

} // namespace pluss::ai::nodeb::embed
