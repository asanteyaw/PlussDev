#include "pluss/ai/nodeb/embed.h"

#include "../io/io_detail.h"
#include "pluss/ai/ice/optim.h"
#include "pluss/nn.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <random>
#include <span>
#include <sstream>
#include <unordered_map>

namespace pluss::ai::nodeb::embed {

namespace {

[[nodiscard]] Config embedding_config() {
  return Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
}

[[nodiscard]] Config index_config() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

[[nodiscard]] std::string select_term(const Token& token, const EmbedOptions& options) {
  const std::string_view piece = (options.use_lemmas && !token.lemma.empty())
                                      ? std::string_view(token.lemma)
                                      : std::string_view(token.orth);
  std::string term(piece);
  if (options.lowercase) {
    std::ranges::transform(term, term.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  }
  return term;
}

[[nodiscard]] std::vector<std::vector<std::string>> docs_to_terms(const std::vector<Doc>& docs,
                                                                   const EmbedOptions& options) {
  std::vector<std::vector<std::string>> out{};
  out.reserve(docs.size());
  for (const auto& doc : docs) {
    std::vector<std::string> terms{};
    terms.reserve(doc.tokens.size());
    for (const auto& token : doc.tokens) {
      if (options.skip_punct && token.is_punct) continue;
      auto term = select_term(token, options);
      if (term.empty()) continue;
      terms.push_back(std::move(term));
    }
    out.push_back(std::move(terms));
  }
  return out;
}

struct VocabCount {
  std::string text{};
  std::int64_t count = 0;
};

[[nodiscard]] std::vector<VocabCount> build_vocab(const std::vector<std::vector<std::string>>& doc_terms,
                                                   std::int64_t min_count) {
  std::unordered_map<std::string, std::int64_t> counts{};
  for (const auto& terms : doc_terms) {
    for (const auto& term : terms) {
      ++counts[term];
    }
  }
  std::vector<VocabCount> out{};
  out.reserve(counts.size());
  for (auto& entry : counts) {
    if (entry.second >= min_count) {
      out.push_back(VocabCount{std::move(entry.first), entry.second});
    }
  }
  std::ranges::sort(out, [](const VocabCount& a, const VocabCount& b) {
    if (a.count != b.count) return a.count > b.count;
    return a.text < b.text;
  });
  return out;
}

struct TrainingPairs {
  std::vector<std::int64_t> centers{};
  std::vector<std::int64_t> contexts{};
};

[[nodiscard]] TrainingPairs build_pairs(const std::vector<std::vector<std::string>>& doc_terms,
                                        const std::unordered_map<std::string, std::int64_t>& token_to_id,
                                        std::int64_t window) {
  TrainingPairs pairs{};
  for (const auto& terms : doc_terms) {
    std::vector<std::int64_t> ids{};
    ids.reserve(terms.size());
    for (const auto& term : terms) {
      const auto it = token_to_id.find(term);
      if (it != token_to_id.end()) {
        ids.push_back(it->second);
      }
    }
    const auto n = static_cast<std::int64_t>(ids.size());
    for (std::int64_t i = 0; i < n; ++i) {
      const auto lo = std::max<std::int64_t>(0, i - window);
      const auto hi = std::min<std::int64_t>(n - 1, i + window);
      for (std::int64_t j = lo; j <= hi; ++j) {
        if (j == i) continue;
        pairs.centers.push_back(ids[static_cast<std::size_t>(i)]);
        pairs.contexts.push_back(ids[static_cast<std::size_t>(j)]);
      }
    }
  }
  return pairs;
}

[[nodiscard]] std::vector<double> build_negative_distribution(const std::vector<VocabCount>& vocab) {
  std::vector<double> probs(vocab.size());
  std::ranges::transform(vocab, probs.begin(),
                         [](const VocabCount& v) { return std::pow(static_cast<double>(v.count), 0.75); });
  return probs;
}

} // namespace

struct Embedder::Impl {
  std::vector<std::string> vocabulary{};
  std::unordered_map<std::string, std::int64_t> token_to_id{};
  std::vector<std::vector<double>> host_vectors{};
  bool trained = false;
};

Embedder::Embedder(EmbedOptions options)
    : impl_(std::make_shared<Impl>()), options_(options) {}

bool Embedder::ready() const noexcept {
  return impl_ && impl_->trained;
}

std::int64_t Embedder::width() const noexcept {
  return options_.width;
}

const std::vector<std::string>& Embedder::vocabulary() const noexcept {
  static const std::vector<std::string> kEmpty{};
  return impl_ ? impl_->vocabulary : kEmpty;
}

bool Embedder::contains(std::string_view token) const noexcept {
  return impl_ && impl_->token_to_id.contains(std::string(token));
}

nodej::Status Embedder::learn(const std::vector<Doc>& docs) {
  PLUSS_CALL_GUARD();
  if (docs.empty()) {
    return nodej::Status::invalid_argument(
        "nlp::embed::Embedder::learn: expected at least one document.");
  }
  if (options_.width <= 0 || options_.window <= 0 || options_.negative_samples <= 0 ||
      options_.min_count <= 0 || options_.epochs <= 0 || options_.batch_size <= 0 ||
      !std::isfinite(options_.learning_rate) || options_.learning_rate <= 0.0) {
    return nodej::Status::invalid_argument(
        "nlp::embed::Embedder::learn: width, window, negative_samples, min_count, epochs, "
        "batch_size, and learning_rate must all be positive.");
  }

  const auto doc_terms = docs_to_terms(docs, options_);
  const auto vocab = build_vocab(doc_terms, options_.min_count);
  if (vocab.empty()) {
    return nodej::Status::precondition(
        "nlp::embed::Embedder::learn: vocabulary is empty after min_count filtering.");
  }
  const auto vocab_size = static_cast<std::int64_t>(vocab.size());

  auto impl = std::make_shared<Impl>();
  impl->vocabulary.reserve(vocab.size());
  impl->token_to_id.reserve(vocab.size());
  for (std::int64_t id = 0; id < vocab_size; ++id) {
    impl->vocabulary.push_back(vocab[static_cast<std::size_t>(id)].text);
    impl->token_to_id.emplace(vocab[static_cast<std::size_t>(id)].text, id);
  }

  const auto pairs = build_pairs(doc_terms, impl->token_to_id, options_.window);
  if (pairs.centers.empty()) {
    return nodej::Status::precondition(
        "nlp::embed::Embedder::learn: no training pairs were generated; documents may be "
        "too short for the configured window, or every token was filtered by min_count.");
  }
  const auto negative_probs = build_negative_distribution(vocab);

  const auto cfg = embedding_config();
  const auto idx_cfg = index_config();
  const double scale = 0.5 / static_cast<double>(options_.width);
  Series input_weights = pluss::series::rand({vocab_size, options_.width}, cfg, options_.seed)
                             .sub(0.5)
                             .mul(2.0 * scale);
  Series output_weights = pluss::series::rand({vocab_size, options_.width}, cfg, options_.seed + 1)
                              .sub(0.5)
                              .mul(2.0 * scale);
  input_weights.tracks_slope_(true);
  output_weights.tracks_slope_(true);

  // Skip-gram negative sampling redraws random negative samples on every
  // mini-batch, so this loop doesn't fit ice::Model's generic forward/loss
  // shape (there is no fixed (x, y) pair) — but it still uses ice's
  // optimizer construction, which wires in gradient clipping and
  // non-finite diagnostics that a hand-rolled `pluss::nn::optim::Adam`
  // construction here previously lacked entirely.
  std::vector<Series*> parameters{&input_weights, &output_weights};
  pluss::ai::ice::TrainOptions optimizer_options{};
  optimizer_options.optimizer = pluss::ai::ice::OptimizerKind::Adam;
  optimizer_options.learning_rate = options_.learning_rate;
  auto optimizer = pluss::ai::ice::make_optimizer(parameters, optimizer_options);

  const auto pair_count = static_cast<std::int64_t>(pairs.centers.size());
  std::vector<std::int64_t> order(static_cast<std::size_t>(pair_count));
  std::iota(order.begin(), order.end(), std::int64_t{0});
  std::mt19937_64 rng(options_.seed);

  try {
    std::uint64_t step = 0;
    for (std::int64_t epoch = 0; epoch < options_.epochs; ++epoch) {
      std::ranges::shuffle(order, rng);
      for (std::int64_t start = 0; start < pair_count; start += options_.batch_size) {
        const auto end = std::min(pair_count, start + options_.batch_size);
        const auto batch_n = end - start;

        std::vector<std::int64_t> center_batch(static_cast<std::size_t>(batch_n));
        std::vector<std::int64_t> context_batch(static_cast<std::size_t>(batch_n));
        for (std::int64_t k = 0; k < batch_n; ++k) {
          const auto idx = static_cast<std::size_t>(order[static_cast<std::size_t>(start + k)]);
          center_batch[static_cast<std::size_t>(k)] = pairs.centers[idx];
          context_batch[static_cast<std::size_t>(k)] = pairs.contexts[idx];
        }

        const Series neg_ids = pluss::series::multinomial(std::span<const double>(negative_probs),
                                                   batch_n * options_.negative_samples,
                                                   /*replacement=*/true, idx_cfg,
                                                   options_.seed + 1000 + step);
        ++step;

        const Series center_series =
            pluss::make_series(center_batch, std::vector<std::int64_t>{batch_n}, idx_cfg);
        const Series context_series =
            pluss::make_series(context_batch, std::vector<std::int64_t>{batch_n}, idx_cfg);

        const Series center_emb = input_weights.index_select(0, center_series);   // [B, W]
        const Series pos_emb = output_weights.index_select(0, context_series);    // [B, W]
        const Series neg_emb = output_weights.index_select(0, neg_ids)
                                   .reshape({batch_n, options_.negative_samples, options_.width});

        const Series pos_logits = center_emb.mul(pos_emb).sum(1);                        // [B]
        const Series neg_logits = center_emb.unsqueeze(1).mul(neg_emb).sum(2);           // [B, K]

        const Series pos_target = pluss::series::ones({batch_n}, cfg);
        const Series neg_target = pluss::series::zeros({batch_n, options_.negative_samples}, cfg);

        const Series loss = pluss::nn::losses::BCEWithLogits(pos_logits, pos_target)
                                .add(pluss::nn::losses::BCEWithLogits(neg_logits, neg_target));

        optimizer->clear();
        loss.reverse();
        optimizer->step();
      }
    }
  } catch (const std::exception& e) {
    return nodej::Status::numerical_failure(
        std::string("nlp::embed::Embedder::learn: training became numerically unstable: ") + e.what());
  }

  const auto flat = input_weights.to(pluss::pCPU, ScalarType::Float64)
                        .reshape({vocab_size * options_.width})
                        .to_vec<double>();
  impl->host_vectors.resize(static_cast<std::size_t>(vocab_size));
  for (std::int64_t id = 0; id < vocab_size; ++id) {
    const auto offset = static_cast<std::size_t>(id * options_.width);
    impl->host_vectors[static_cast<std::size_t>(id)].assign(
        flat.begin() + static_cast<std::ptrdiff_t>(offset),
        flat.begin() + static_cast<std::ptrdiff_t>(offset + static_cast<std::size_t>(options_.width)));
  }
  impl->trained = true;
  impl_ = std::move(impl);
  return nodej::Status::success();
}

nodej::StatusOr<std::vector<double>> Embedder::vector_of(std::string_view token) const {
  if (!ready()) {
    return nodej::StatusOr<std::vector<double>>(
        nodej::Status::precondition("nlp::embed::Embedder::vector_of: model has not been trained."));
  }
  const auto it = impl_->token_to_id.find(std::string(token));
  if (it == impl_->token_to_id.end()) {
    return nodej::StatusOr<std::vector<double>>(nodej::Status::invalid_argument(
        "nlp::embed::Embedder::vector_of: '" + std::string(token) + "' is out of vocabulary."));
  }
  return nodej::StatusOr<std::vector<double>>(impl_->host_vectors[static_cast<std::size_t>(it->second)]);
}

nodej::StatusOr<std::vector<double>> Embedder::embed(const Doc& doc) const {
  if (!ready()) {
    return nodej::StatusOr<std::vector<double>>(
        nodej::Status::precondition("nlp::embed::Embedder::embed: model has not been trained."));
  }
  std::vector<double> sum(static_cast<std::size_t>(options_.width), 0.0);
  std::int64_t count = 0;
  for (const auto& token : doc.tokens) {
    if (options_.skip_punct && token.is_punct) continue;
    const auto term = select_term(token, options_);
    const auto it = impl_->token_to_id.find(term);
    if (it == impl_->token_to_id.end()) continue;
    const auto& vec = impl_->host_vectors[static_cast<std::size_t>(it->second)];
    for (std::size_t i = 0; i < vec.size(); ++i) {
      sum[i] += vec[i];
    }
    ++count;
  }
  if (count == 0) {
    return nodej::StatusOr<std::vector<double>>(nodej::Status::precondition(
        "nlp::embed::Embedder::embed: none of the document's tokens are in vocabulary."));
  }
  for (auto& value : sum) {
    value /= static_cast<double>(count);
  }
  return nodej::StatusOr<std::vector<double>>(std::move(sum));
}

nodej::StatusOr<std::vector<std::vector<double>>> Embedder::embed(const std::vector<Doc>& docs) const {
  std::vector<std::vector<double>> out{};
  out.reserve(docs.size());
  for (const auto& doc : docs) {
    auto vector_or = embed(doc);
    if (!vector_or.ok()) {
      return nodej::StatusOr<std::vector<std::vector<double>>>(vector_or.status());
    }
    out.push_back(vector_or.move_value());
  }
  return nodej::StatusOr<std::vector<std::vector<double>>>(std::move(out));
}

double Embedder::similarity(const std::vector<double>& lhs, const std::vector<double>& rhs) noexcept {
  if (lhs.size() != rhs.size() || lhs.empty()) return 0.0;
  double dot = 0.0;
  double lhs_norm = 0.0;
  double rhs_norm = 0.0;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    dot += lhs[i] * rhs[i];
    lhs_norm += lhs[i] * lhs[i];
    rhs_norm += rhs[i] * rhs[i];
  }
  if (lhs_norm <= 0.0 || rhs_norm <= 0.0) return 0.0;
  return dot / (std::sqrt(lhs_norm) * std::sqrt(rhs_norm));
}

nodej::StatusOr<std::vector<std::pair<std::string, double>>> Embedder::nearest(std::string_view token,
                                                                               std::int64_t k) const {
  using Result = std::vector<std::pair<std::string, double>>;
  if (!ready()) {
    return nodej::StatusOr<Result>(
        nodej::Status::precondition("nlp::embed::Embedder::nearest: model has not been trained."));
  }
  if (k < 0) {
    return nodej::StatusOr<Result>(nodej::Status::invalid_argument(
        "nlp::embed::Embedder::nearest: k must be non-negative."));
  }
  const auto it = impl_->token_to_id.find(std::string(token));
  if (it == impl_->token_to_id.end()) {
    return nodej::StatusOr<Result>(nodej::Status::invalid_argument(
        "nlp::embed::Embedder::nearest: '" + std::string(token) + "' is out of vocabulary."));
  }
  const auto query_id = static_cast<std::size_t>(it->second);
  const auto& query_vec = impl_->host_vectors[query_id];

  Result scored{};
  scored.reserve(impl_->vocabulary.size());
  for (std::size_t id = 0; id < impl_->vocabulary.size(); ++id) {
    if (id == query_id) continue;
    scored.emplace_back(impl_->vocabulary[id], similarity(query_vec, impl_->host_vectors[id]));
  }
  const auto count = std::min(static_cast<std::size_t>(k), scored.size());
  std::partial_sort(scored.begin(), scored.begin() + static_cast<std::ptrdiff_t>(count), scored.end(),
                    [](const auto& a, const auto& b) { return a.second > b.second; });
  scored.resize(count);
  return nodej::StatusOr<Result>(std::move(scored));
}

namespace {

constexpr std::string_view kMagicLine = "pluss_nodeb_embedder";
constexpr std::int64_t kCurrentSchema = 2;

[[nodiscard]] std::string hex_encode(std::string_view value) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out{};
  out.reserve(value.size() * 2);
  for (const unsigned char byte : value) {
    out.push_back(kHex[byte >> 4]);
    out.push_back(kHex[byte & 0x0F]);
  }
  return out;
}

[[nodiscard]] std::string hex_decode(std::string_view value) {
  const auto nibble = [](unsigned char c) -> unsigned char {
    return (c >= '0' && c <= '9') ? static_cast<unsigned char>(c - '0')
                                  : static_cast<unsigned char>(c - 'a' + 10);
  };
  std::string out{};
  out.reserve(value.size() / 2);
  for (std::size_t i = 0; i + 1 < value.size(); i += 2) {
    out.push_back(static_cast<char>((nibble(static_cast<unsigned char>(value[i])) << 4) |
                                    nibble(static_cast<unsigned char>(value[i + 1]))));
  }
  return out;
}

[[nodiscard]] std::string join_doubles(const std::vector<double>& values) {
  std::ostringstream oss{};
  oss << std::setprecision(17);
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) oss << ' ';
    oss << values[i];
  }
  return oss.str();
}

[[nodiscard]] std::vector<double> parse_doubles(std::string_view line) {
  std::vector<double> out{};
  std::istringstream iss{std::string(line)};
  double value = 0.0;
  while (iss >> value) {
    out.push_back(value);
  }
  return out;
}

} // namespace

void Embedder::save(std::string_view filepath) const {
  PLUSS_CALL_GUARD();
  if (filepath.empty()) {
    ::pluss::ai::nodeb::detail::fail("nlp::embed::Embedder::save: output path cannot be empty.");
  }
  if (!ready()) {
    ::pluss::ai::nodeb::detail::fail("nlp::embed::Embedder::save: model has not been trained.");
  }
  std::ofstream out{std::string(filepath)};
  if (!out.good()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::embed::Embedder::save: failed to open '" + std::string(filepath) + "'.");
  }

  std::ostringstream body{};
  for (std::size_t id = 0; id < impl_->vocabulary.size(); ++id) {
    body << "term=" << hex_encode(impl_->vocabulary[id]) << "\n";
    body << "vec=" << join_doubles(impl_->host_vectors[id]) << "\n";
  }
  const auto body_text = body.str();

  out << kMagicLine << "\n";
  out << "schema=" << kCurrentSchema << "\n";
  out << "width=" << options_.width << "\n";
  out << "use_lemmas=" << (options_.use_lemmas ? 1 : 0) << "\n";
  out << "lowercase=" << (options_.lowercase ? 1 : 0) << "\n";
  out << "skip_punct=" << (options_.skip_punct ? 1 : 0) << "\n";
  out << "vocab_count=" << impl_->vocabulary.size() << "\n";
  out << "checksum=" << io::detail::to_hex(io::detail::fnv1a64(body_text)) << "\n";
  out << body_text;

  if (!out.good()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::embed::Embedder::save: failed while writing '" + std::string(filepath) + "'.");
  }
}

Embedder Embedder::load(std::string_view filepath) {
  PLUSS_CALL_GUARD();
  if (filepath.empty()) {
    ::pluss::ai::nodeb::detail::fail("nlp::embed::Embedder::load: input path cannot be empty.");
  }
  std::ifstream in{std::string(filepath)};
  if (!in.good()) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::embed::Embedder::load: failed to open '" + std::string(filepath) + "'.");
  }

  std::string line{};
  if (!std::getline(in, line) || line != kMagicLine) {
    ::pluss::ai::nodeb::detail::fail("nlp::embed::Embedder::load: invalid embedder header.");
  }

  std::int64_t schema = -1;
  std::int64_t width = -1;
  bool use_lemmas = true;
  bool lowercase = true;
  bool skip_punct = true;
  std::int64_t vocab_count = -1;
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
    if (line.rfind("use_lemmas=", 0) == 0) {
      use_lemmas = io::detail::parse_i64(std::string_view(line).substr(11)) != 0;
      continue;
    }
    if (line.rfind("lowercase=", 0) == 0) {
      lowercase = io::detail::parse_i64(std::string_view(line).substr(10)) != 0;
      continue;
    }
    if (line.rfind("skip_punct=", 0) == 0) {
      skip_punct = io::detail::parse_i64(std::string_view(line).substr(11)) != 0;
      continue;
    }
    if (line.rfind("vocab_count=", 0) == 0) {
      vocab_count = io::detail::parse_i64(std::string_view(line).substr(12));
      continue;
    }
    if (line.rfind("checksum=", 0) == 0) {
      expected_checksum = io::detail::parse_hex_u64(std::string_view(line).substr(9));
      have_checksum = true;
      break; // checksum is always the last header line; the body follows immediately.
    }
    ::pluss::ai::nodeb::detail::fail(
        "nlp::embed::Embedder::load: unrecognized embedder header line '" + line + "'.");
  }
  if (schema != 1 && schema != kCurrentSchema) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::embed::Embedder::load: unsupported embedder schema " + std::to_string(schema) + ".");
  }
  if (width <= 0 || vocab_count < 0 || !have_checksum) {
    ::pluss::ai::nodeb::detail::fail("nlp::embed::Embedder::load: embedder header is incomplete.");
  }

  std::ostringstream body_buffer{};
  body_buffer << in.rdbuf();
  const auto body_text = body_buffer.str();
  if (io::detail::fnv1a64(body_text) != expected_checksum) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::embed::Embedder::load: checksum mismatch, artifact is truncated or corrupted.");
  }

  EmbedOptions options{};
  options.width = width;
  options.use_lemmas = use_lemmas;
  options.lowercase = lowercase;
  options.skip_punct = skip_punct;
  Embedder embedder(options);
  auto impl = std::make_shared<Impl>();
  impl->vocabulary.reserve(static_cast<std::size_t>(vocab_count));
  impl->host_vectors.reserve(static_cast<std::size_t>(vocab_count));

  std::istringstream body_stream(body_text);
  std::string term_line{};
  std::string vec_line{};
  while (std::getline(body_stream, term_line)) {
    if (term_line.rfind("term=", 0) != 0 || !std::getline(body_stream, vec_line) ||
        vec_line.rfind("vec=", 0) != 0) {
      ::pluss::ai::nodeb::detail::fail("nlp::embed::Embedder::load: malformed vocabulary entry.");
    }
    auto term = hex_decode(std::string_view(term_line).substr(5));
    auto vec = parse_doubles(std::string_view(vec_line).substr(4));
    if (static_cast<std::int64_t>(vec.size()) != width) {
      ::pluss::ai::nodeb::detail::fail(
          "nlp::embed::Embedder::load: vector width mismatch for '" + term + "'.");
    }
    impl->token_to_id.emplace(term, static_cast<std::int64_t>(impl->vocabulary.size()));
    impl->vocabulary.push_back(std::move(term));
    impl->host_vectors.push_back(std::move(vec));
  }
  if (static_cast<std::int64_t>(impl->vocabulary.size()) != vocab_count) {
    ::pluss::ai::nodeb::detail::fail(
        "nlp::embed::Embedder::load: expected " + std::to_string(vocab_count) +
        " vocabulary entries but found " + std::to_string(impl->vocabulary.size()) + ".");
  }
  impl->trained = true;
  embedder.impl_ = std::move(impl);
  return embedder;
}

} // namespace pluss::ai::nodeb::embed
