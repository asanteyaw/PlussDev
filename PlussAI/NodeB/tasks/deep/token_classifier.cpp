#include "token_classifier.h"

#include "sequence_heads.h"

#include "pluss/ai/ice/optim.h"
#include "pluss/nn.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace pluss::ai::nodeb::detail {

namespace {

[[nodiscard]] Config token_classifier_float_cfg() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

// Collects `<prefix><index>=<value>` lines into an index-ordered vector.
[[nodiscard]] std::vector<std::string> collect_indexed_lines(
    const std::string& prefix,
    std::int64_t count,
    pluss::msharp::RangeView::Of<std::string_view> lines) {
  std::vector<std::string> out(static_cast<std::size_t>(std::max<std::int64_t>(0, count)));
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string_view line = lines[i];
    if (line.rfind(prefix, 0) != 0) continue;
    const std::string_view rest = line.substr(prefix.size());
    std::size_t digits = 0;
    while (digits < rest.size() && rest[digits] >= '0' && rest[digits] <= '9') ++digits;
    if (digits == 0 || digits >= rest.size() || rest[digits] != '=') continue;
    const auto idx = static_cast<std::size_t>(std::stoll(std::string(rest.substr(0, digits))));
    if (idx >= out.size()) continue;
    out[idx] = std::string(rest.substr(digits + 1));
  }
  return out;
}

} // namespace

void train_token_classifier(TokenClassifierState& state,
                            const std::vector<Doc>& docs,
                            const std::vector<std::vector<std::string>>& token_labels,
                            const TokenClassifierOptions& options) {
  PLUSS_CALL_GUARD();
  state = TokenClassifierState{};
  if (docs.empty() || docs.size() != token_labels.size()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::train_token_classifier: docs and token_labels must be the same non-empty size.");
  }
  if (options.width <= 0 || options.num_layers <= 0 || options.num_heads <= 0 ||
      options.feedforward_width <= 0 || options.epochs <= 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::train_token_classifier: width, num_layers, num_heads, feedforward_width "
        "and epochs must all be positive.");
  }
  if (options.width % options.num_heads != 0) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::train_token_classifier: width must be a multiple of num_heads.");
  }

  std::vector<std::string> labels{};
  std::unordered_map<std::string, std::int64_t> label_to_index{};
  for (const auto& doc_labels : token_labels) {
    for (const auto& label : doc_labels) {
      if (label.empty()) continue;
      if (!label_to_index.contains(label)) {
        label_to_index.emplace(label, static_cast<std::int64_t>(labels.size()));
        labels.push_back(label);
      }
    }
  }
  if (labels.size() < 2) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::detail::train_token_classifier: expected at least two distinct labels.");
  }

  runtime::SequenceOptions seq_options{};
  seq_options.use_lemmas = options.use_lemmas;
  seq_options.skip_punct = false; // every token needs a slot, including punctuation.
  seq_options.lowercase = options.lowercase;
  seq_options.add_bos = false;
  seq_options.add_eos = false;
  seq_options.batch_first = true;

  runtime::Indexer indexer(seq_options);
  const auto indexer_status = indexer.learn(docs);
  if (!indexer_status.ok()) {
    ::pluss::ai::nodeb::detail::fail("nlp::detail::train_token_classifier: " + indexer_status.message);
  }
  const auto batch_or = indexer.batch(docs);
  if (!batch_or.ok()) {
    ::pluss::ai::nodeb::detail::fail("nlp::detail::train_token_classifier: " + batch_or.status().message);
  }
  const auto& batch = batch_or.value();
  if (batch.empty()) {
    ::pluss::ai::nodeb::detail::fail("nlp::detail::train_token_classifier: documents produced no tokens.");
  }

  const auto vocab_size = static_cast<std::int64_t>(indexer.vocabulary().size());
  const auto width = options.width;
  const auto cfg = token_classifier_float_cfg();
  const auto num_classes = static_cast<std::int64_t>(labels.size());
  const auto n = batch.batch_size;
  const auto t = batch.max_length;

  auto embedding = std::make_shared<runtime::EmbeddingTable>(runtime::EmbeddingOptions{
      .width = width,
      .use_positions = true,
      .learned_positions = true,
      .max_positions = std::max<std::int64_t>(t + 1, 512),
      .cfg = cfg,
      .seed = options.seed,
  });
  embedding->reset(vocab_size, indexer.pad_id());
  embedding->weights().tracks_slope_(true);
  embedding->position_weights().tracks_slope_(true);

  auto encoder = std::make_shared<runtime::TransformerTextEncoder>(pluss::nn::TransformerEncoderOptions{
      .d_model = width,
      .nhead = options.num_heads,
      .num_layers = options.num_layers,
      .dim_feedforward = options.feedforward_width,
      .dropout = 0.1,
      .batch_first = true,
      .cfg = cfg,
      .seed = options.seed + 1,
  });
  encoder->model()->train(true);

  const double scale = 0.5 / static_cast<double>(width);
  Series cls_weight = pluss::series::rand({width, num_classes}, cfg, options.seed + 2).sub(0.5).mul(2.0 * scale);
  Series cls_bias = pluss::series::zeros({num_classes}, cfg);
  cls_weight.tracks_slope_(true);
  cls_bias.tracks_slope_(true);

  std::vector<Series*> parameters = encoder->model()->parameters();
  parameters.push_back(&const_cast<Series&>(embedding->weights()));
  parameters.push_back(&const_cast<Series&>(embedding->position_weights()));
  parameters.push_back(&cls_weight);
  parameters.push_back(&cls_bias);

  // The architecture (embedding table sized off a data-discovered vocab,
  // structured SequenceBatch input, one full-batch pass per epoch) doesn't
  // fit ice::Model's generic forward/loss shape — but ice::make_optimizer
  // still applies cleanly, and matches what was already hand-rolled here
  // (ByNorm/1.0 clipping, fail-loud non-finite diagnostics) exactly, so this
  // is a pure de-duplication onto the shared construction path rather than a
  // behavior change.
  pluss::ai::ice::TrainOptions optimizer_options{};
  optimizer_options.optimizer = pluss::ai::ice::OptimizerKind::Adam;
  optimizer_options.learning_rate = options.learning_rate;
  auto optimizer = pluss::ai::ice::make_optimizer(parameters, optimizer_options);

  // -100 (CrossEntropy's ignore_index) marks padding and untagged
  // positions so they contribute no loss/gradient.
  std::vector<std::int64_t> targets(static_cast<std::size_t>(n * t), -100);
  for (std::int64_t d = 0; d < n; ++d) {
    const auto& doc_labels = token_labels[static_cast<std::size_t>(d)];
    for (std::int64_t pos = 0; pos < t && pos < static_cast<std::int64_t>(doc_labels.size()); ++pos) {
      const auto& label = doc_labels[static_cast<std::size_t>(pos)];
      if (label.empty()) continue;
      targets[static_cast<std::size_t>(d * t + pos)] = label_to_index.at(label);
    }
  }
  const Series target_series = pluss::make_series(
      targets, std::vector<std::int64_t>{n, t}, Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU));

  pluss::nn::losses::CrossEntropyOptions ce_options{};
  ce_options.use_ignore_index = true;
  ce_options.ignore_index = -100;

  state.loss_history.clear();
  state.loss_history.reserve(static_cast<std::size_t>(options.epochs));
  try {
    for (std::int64_t epoch = 0; epoch < options.epochs; ++epoch) {
      const Series embedded = embedding->forward(batch);
      const Series encoded = encoder->forward(embedded, batch, /*causal=*/false);
      const Series logits = encoded.reshape({n * t, width})
                                .matmul(cls_weight)
                                .add(cls_bias)
                                .reshape({n, t, num_classes});
      const Series loss = pluss::nn::losses::CrossEntropy(logits, target_series, ce_options);

      optimizer->clear();
      loss.reverse();
      optimizer->step();
      state.loss_history.push_back(loss.to(pluss::pCPU, ScalarType::Float64).item<double>());
    }
  } catch (const std::exception& e) {
    ::pluss::ai::nodeb::detail::fail(
        std::string("nlp::detail::train_token_classifier: training became numerically unstable: ") + e.what());
  }

  encoder->model()->eval();
  embedding->weights().tracks_slope_(false);
  embedding->position_weights().tracks_slope_(false);
  cls_weight.tracks_slope_(false);
  cls_bias.tracks_slope_(false);

  state.indexer = std::move(indexer);
  state.embedding = std::move(embedding);
  state.encoder = std::move(encoder);
  state.cls_weight = std::move(cls_weight);
  state.cls_bias = std::move(cls_bias);
  state.labels = std::move(labels);
  state.width = width;
  state.num_layers = options.num_layers;
  state.num_heads = options.num_heads;
  state.feedforward_width = options.feedforward_width;
  state.ready = true;
}

std::vector<std::pair<std::string, double>> predict_token_labels(const TokenClassifierState& state,
                                                                  const Doc& doc) {
  if (!state.ready) return {};
  const auto batch_or = state.indexer.batch(std::vector<Doc>{doc});
  if (!batch_or.ok()) return {};
  const auto& batch = batch_or.value();
  if (batch.empty()) return {};

  const Series embedded = state.embedding->forward(batch);
  const Series encoded = state.encoder->forward(embedded, batch, /*causal=*/false);
  const auto length = batch.max_length;
  const auto num_classes = static_cast<std::int64_t>(state.labels.size());
  const Series logits =
      encoded.reshape({length, state.width}).matmul(state.cls_weight).add(state.cls_bias); // [length, classes]
  const auto flat =
      logits.to(pluss::pCPU, ScalarType::Float64).reshape({length * num_classes}).to_vec<double>();

  std::vector<std::pair<std::string, double>> out{};
  const auto token_count = std::min<std::int64_t>(length, static_cast<std::int64_t>(doc.tokens.size()));
  out.reserve(static_cast<std::size_t>(token_count));
  for (std::int64_t pos = 0; pos < token_count; ++pos) {
    const auto offset = static_cast<std::size_t>(pos * num_classes);
    double best = flat[offset];
    std::int64_t best_idx = 0;
    for (std::int64_t c = 1; c < num_classes; ++c) {
      const auto value = flat[offset + static_cast<std::size_t>(c)];
      if (value > best) {
        best = value;
        best_idx = c;
      }
    }
    double denom = 0.0;
    for (std::int64_t c = 0; c < num_classes; ++c) {
      denom += std::exp(flat[offset + static_cast<std::size_t>(c)] - best);
    }
    out.emplace_back(state.labels[static_cast<std::size_t>(best_idx)], 1.0 / denom);
  }
  return out;
}

std::vector<std::string> dump_token_classifier_state_lines(const char* prefix, const TokenClassifierState& state) {
  PLUSS_CALL_GUARD();
  std::vector<std::string> lines{};
  const std::string p(prefix);
  lines.push_back(p + "_ready=" + (state.ready ? "1" : "0"));
  if (!state.ready) {
    lines.push_back(p + "_width=0");
    lines.push_back(p + "_num_layers=0");
    lines.push_back(p + "_num_heads=0");
    lines.push_back(p + "_feedforward_width=0");
    lines.push_back(p + "_label_count=0");
    lines.push_back(p + "_indexer_line_count=0");
    lines.push_back(p + "_embedding_line_count=0");
    lines.push_back(p + "_cls_weight=");
    lines.push_back(p + "_cls_bias=");
    lines.push_back(p + "_encoder_param_count=0");
    return lines;
  }

  lines.push_back(p + "_width=" + std::to_string(state.width));
  lines.push_back(p + "_num_layers=" + std::to_string(state.num_layers));
  lines.push_back(p + "_num_heads=" + std::to_string(state.num_heads));
  lines.push_back(p + "_feedforward_width=" + std::to_string(state.feedforward_width));
  lines.push_back(p + "_label_count=" + std::to_string(state.labels.size()));
  for (std::size_t i = 0; i < state.labels.size(); ++i) {
    lines.push_back(p + "_label_" + std::to_string(i) + "=" + hex_encode_string(state.labels[i]));
  }

  const auto indexer_lines = state.indexer.dump_state_lines();
  lines.push_back(p + "_indexer_line_count=" + std::to_string(indexer_lines.size()));
  for (std::size_t i = 0; i < indexer_lines.size(); ++i) {
    lines.push_back(p + "_indexer_" + std::to_string(i) + "=" + indexer_lines[i]);
  }

  const auto embedding_lines = state.embedding->dump_state_lines();
  lines.push_back(p + "_embedding_line_count=" + std::to_string(embedding_lines.size()));
  for (std::size_t i = 0; i < embedding_lines.size(); ++i) {
    lines.push_back(p + "_embedding_" + std::to_string(i) + "=" + embedding_lines[i]);
  }

  const auto cls_weight_flat =
      state.cls_weight.to(pluss::pCPU, ScalarType::Float64).reshape({state.cls_weight.numel()}).to_vec<double>();
  const auto cls_bias_flat =
      state.cls_bias.to(pluss::pCPU, ScalarType::Float64).reshape({state.cls_bias.numel()}).to_vec<double>();
  lines.push_back(p + "_cls_weight=" + join_doubles_line(cls_weight_flat));
  lines.push_back(p + "_cls_bias=" + join_doubles_line(cls_bias_flat));

  const auto params = state.encoder->model()->parameters();
  lines.push_back(p + "_encoder_param_count=" + std::to_string(params.size()));
  for (std::size_t i = 0; i < params.size(); ++i) {
    const auto shape_view = params[i]->sizes();
    const std::vector<double> shape_as_double(shape_view.begin(), shape_view.end());
    const auto flat = params[i]->to(pluss::pCPU, ScalarType::Float64).reshape({params[i]->numel()}).to_vec<double>();
    lines.push_back(p + "_encoder_param_" + std::to_string(i) + "_shape=" + join_doubles_line(shape_as_double));
    lines.push_back(p + "_encoder_param_" + std::to_string(i) + "=" + join_doubles_line(flat));
  }
  return lines;
}

void load_token_classifier_state_lines(const char* prefix,
                                       pluss::msharp::RangeView::Of<std::string_view> lines,
                                       TokenClassifierState& state) {
  PLUSS_CALL_GUARD();
  state = TokenClassifierState{};
  const std::string p(prefix);

  bool ready = false;
  std::int64_t width = 0;
  std::int64_t num_layers = 0;
  std::int64_t num_heads = 0;
  std::int64_t feedforward_width = 0;
  std::int64_t label_count = -1;
  std::vector<std::string> labels{};
  std::int64_t indexer_line_count = -1;
  std::int64_t embedding_line_count = -1;
  std::int64_t encoder_param_count = -1;
  std::vector<double> cls_weight_flat{};
  std::vector<double> cls_bias_flat{};

  const std::string label_prefix = p + "_label_";
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string_view line = lines[i];
    const auto eq = line.find('=');
    if (eq == std::string_view::npos) continue;
    const std::string key(line.substr(0, eq));
    const std::string_view value = line.substr(eq + 1);
    if (key == p + "_ready") { ready = value == "1"; continue; }
    if (key == p + "_width") { width = std::stoll(std::string(value)); continue; }
    if (key == p + "_num_layers") { num_layers = std::stoll(std::string(value)); continue; }
    if (key == p + "_num_heads") { num_heads = std::stoll(std::string(value)); continue; }
    if (key == p + "_feedforward_width") { feedforward_width = std::stoll(std::string(value)); continue; }
    if (key == p + "_label_count") {
      label_count = std::stoll(std::string(value));
      labels.assign(static_cast<std::size_t>(std::max<std::int64_t>(0, label_count)), {});
      continue;
    }
    if (key.rfind(label_prefix, 0) == 0) {
      const auto idx_str = key.substr(label_prefix.size());
      if (!idx_str.empty() && std::all_of(idx_str.begin(), idx_str.end(),
                                          [](char c) { return c >= '0' && c <= '9'; })) {
        const auto idx = static_cast<std::size_t>(std::stoll(idx_str));
        if (idx < labels.size()) labels[idx] = hex_decode_string(value);
      }
      continue;
    }
    if (key == p + "_indexer_line_count") { indexer_line_count = std::stoll(std::string(value)); continue; }
    if (key == p + "_embedding_line_count") { embedding_line_count = std::stoll(std::string(value)); continue; }
    if (key == p + "_encoder_param_count") { encoder_param_count = std::stoll(std::string(value)); continue; }
    if (key == p + "_cls_weight") { cls_weight_flat = parse_doubles_line(value); continue; }
    if (key == p + "_cls_bias") { cls_bias_flat = parse_doubles_line(value); continue; }
  }

  if (!ready) {
    return;
  }
  if (label_count < 2 || width <= 0 || num_layers <= 0 || num_heads <= 0 || feedforward_width <= 0 ||
      indexer_line_count < 0 || embedding_line_count < 0 || encoder_param_count < 0) {
    ::pluss::ai::nodeb::detail::fail(
        std::string("nlp::detail::load_token_classifier_state_lines: malformed ") + prefix + " state.");
  }

  const auto indexer_lines_raw = collect_indexed_lines(p + "_indexer_", indexer_line_count, lines);
  const std::vector<std::string_view> indexer_views(indexer_lines_raw.begin(), indexer_lines_raw.end());
  runtime::Indexer indexer{};
  const auto indexer_status = indexer.load_state_lines(pluss::msharp::RangeView::Of<std::string_view>(indexer_views));
  if (!indexer_status.ok()) {
    ::pluss::ai::nodeb::detail::fail(
        std::string("nlp::detail::load_token_classifier_state_lines: ") + indexer_status.message);
  }

  const auto embedding_lines_raw = collect_indexed_lines(p + "_embedding_", embedding_line_count, lines);
  const std::vector<std::string_view> embedding_views(embedding_lines_raw.begin(), embedding_lines_raw.end());
  auto embedding = std::make_shared<runtime::EmbeddingTable>();
  const auto embedding_status =
      embedding->load_state_lines(pluss::msharp::RangeView::Of<std::string_view>(embedding_views));
  if (!embedding_status.ok()) {
    ::pluss::ai::nodeb::detail::fail(
        std::string("nlp::detail::load_token_classifier_state_lines: ") + embedding_status.message);
  }

  const auto cfg = token_classifier_float_cfg();
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
        std::string("nlp::detail::load_token_classifier_state_lines: ") + prefix +
        " encoder parameter count mismatch (architecture options changed since save?).");
  }

  std::vector<std::string> shape_lines(static_cast<std::size_t>(encoder_param_count));
  std::vector<std::string> value_lines(static_cast<std::size_t>(encoder_param_count));
  const std::string param_prefix = p + "_encoder_param_";
  for (std::size_t i = 0; i < lines.size(); ++i) {
    const std::string_view line = lines[i];
    if (line.rfind(param_prefix, 0) != 0) continue;
    const std::string_view rest = line.substr(param_prefix.size());
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
    const auto shape_doubles = parse_doubles_line(shape_lines[idx]);
    const std::vector<std::int64_t> shape(shape_doubles.begin(), shape_doubles.end());
    auto values = parse_doubles_line(value_lines[idx]);
    *encoder_params[idx] = pluss::make_series(values, shape, cfg).to(cfg.platform, cfg.dtype);
  }

  const auto num_classes = static_cast<std::int64_t>(labels.size());
  if (static_cast<std::int64_t>(cls_weight_flat.size()) != width * num_classes ||
      static_cast<std::int64_t>(cls_bias_flat.size()) != num_classes) {
    ::pluss::ai::nodeb::detail::fail(
        std::string("nlp::detail::load_token_classifier_state_lines: ") + prefix +
        " classifier head shape mismatch.");
  }
  Series cls_weight = pluss::make_series(cls_weight_flat, std::vector<std::int64_t>{width, num_classes}, cfg)
                          .to(cfg.platform, cfg.dtype);
  Series cls_bias =
      pluss::make_series(cls_bias_flat, std::vector<std::int64_t>{num_classes}, cfg).to(cfg.platform, cfg.dtype);

  state.indexer = std::move(indexer);
  state.embedding = std::move(embedding);
  state.encoder = std::move(encoder);
  state.cls_weight = std::move(cls_weight);
  state.cls_bias = std::move(cls_bias);
  state.labels = std::move(labels);
  state.width = width;
  state.num_layers = num_layers;
  state.num_heads = num_heads;
  state.feedforward_width = feedforward_width;
  state.ready = true;
}

} // namespace pluss::ai::nodeb::detail
