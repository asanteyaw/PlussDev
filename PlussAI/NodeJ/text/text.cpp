#include "pluss/ai/nodej/text.h"

#include "CodeN/native/cpu/TextKernels.h"
#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace pluss::ai::nodej {
namespace {

[[nodiscard]] Config f64_cpu() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

[[nodiscard]] Config i64_cpu() {
  return Config{}.with_dtype(ScalarType::Int64).with_platform(pluss::pCPU);
}

[[nodiscard]] std::string normalized_token(const std::string& token, bool lowercase) {
  if (!lowercase) return token;
  return ::pluss::coden::native::cpu::lower_ascii(token);
}

[[nodiscard]] Status validate_ngram_range(std::int64_t min_ngram,
                                          std::int64_t max_ngram,
                                          std::string_view where) {
  if (min_ngram <= 0 || max_ngram <= 0) {
    return Status::invalid_argument(std::string(where) + " requires min_ngram and max_ngram to be > 0.");
  }
  if (min_ngram > max_ngram) {
    return Status::invalid_argument(std::string(where) + " requires min_ngram <= max_ngram.");
  }
  return Status::success();
}

[[nodiscard]] std::vector<std::string> tokenize_terms(const std::string& text, bool lowercase, bool keep_punct) {
  const std::string normalized =
      lowercase ? ::pluss::coden::native::cpu::lower_ascii(text) : text;
  const auto spans = keep_punct ? ::pluss::coden::native::cpu::split_nlp_token_spans(normalized, true)
                                : ::pluss::coden::native::cpu::split_word_spans(normalized);
  std::vector<std::string> tokens{};
  tokens.reserve(spans.size());
  for (const auto& [start, len] : spans) {
    tokens.push_back(normalized.substr(start, len));
  }
  return tokens;
}

[[nodiscard]] std::vector<std::string> build_ngrams(const std::vector<std::string>& tokens,
                                                    std::int64_t min_ngram,
                                                    std::int64_t max_ngram) {
  std::vector<std::string> out{};
  if (tokens.empty()) return out;
  const auto capped_max = std::min<std::int64_t>(max_ngram, static_cast<std::int64_t>(tokens.size()));
  for (std::int64_t n = min_ngram; n <= capped_max; ++n) {
    for (std::int64_t i = 0; i + n <= static_cast<std::int64_t>(tokens.size()); ++i) {
      std::string gram = tokens[static_cast<std::size_t>(i)];
      for (std::int64_t j = 1; j < n; ++j) {
        gram.push_back(' ');
        gram += tokens[static_cast<std::size_t>(i + j)];
      }
      out.push_back(std::move(gram));
    }
  }
  return out;
}

[[nodiscard]] std::vector<std::string> analyze_terms(const std::string& text,
                                                     bool lowercase,
                                                     bool keep_punct,
                                                     std::int64_t min_ngram,
                                                     std::int64_t max_ngram) {
  return build_ngrams(tokenize_terms(text, lowercase, keep_punct), min_ngram, max_ngram);
}

[[nodiscard]] std::uint64_t stable_hash64(std::string_view value) noexcept {
  constexpr std::uint64_t kOffset = 1469598103934665603ULL;
  constexpr std::uint64_t kPrime = 1099511628211ULL;
  std::uint64_t hash = kOffset;
  for (unsigned char ch : value) {
    hash ^= ch;
    hash *= kPrime;
  }
  return hash;
}

} // namespace

LexiconMapper::LexiconMapper(LexiconMapOptions options) : options_(options) {}

LexiconMapperState LexiconMapper::dump_state() const {
  return LexiconMapperState{
      .options = options_,
      .learned = learned_,
      .learned_document_count = learned_document_count_,
      .vocabulary = vocabulary_,
      .document_frequency = document_frequency_,
  };
}

Status LexiconMapper::load_state(const LexiconMapperState& state) {
  if (state.options.max_features <= 0) {
    return Status::invalid_argument("LexiconMapper::load_state requires max_features > 0.");
  }
  if (state.options.min_document_frequency <= 0) {
    return Status::invalid_argument("LexiconMapper::load_state requires min_document_frequency > 0.");
  }
  if (const auto status =
          validate_ngram_range(state.options.min_ngram, state.options.max_ngram, "LexiconMapper::load_state");
      !status.ok()) {
    return status;
  }
  if (state.vocabulary.size() != state.document_frequency.size()) {
    return Status::shape_mismatch("LexiconMapper::load_state vocabulary/document_frequency size mismatch.");
  }
  if (state.learned && state.vocabulary.empty()) {
    return Status::invalid_argument("LexiconMapper::load_state learned state requires a non-empty vocabulary.");
  }

  options_ = state.options;
  learned_ = state.learned;
  learned_document_count_ = state.learned_document_count;
  vocabulary_ = state.vocabulary;
  document_frequency_ = state.document_frequency;
  return Status::success();
}

Status LexiconMapper::learn(pluss::msharp::RangeView::Of<std::string> documents) {
  if (documents.empty()) return Status::invalid_argument("LexiconMapper::learn requires at least one document.");
  if (options_.max_features <= 0) {
    return Status::invalid_argument("LexiconMapOptions.max_features must be > 0.");
  }
  if (options_.min_document_frequency <= 0) {
    return Status::invalid_argument("LexiconMapOptions.min_document_frequency must be > 0.");
  }
  if (const auto status =
          validate_ngram_range(options_.min_ngram, options_.max_ngram, "LexiconMapper::learn");
      !status.ok()) {
    return status;
  }

  learned_document_count_ = static_cast<std::int64_t>(documents.size());
  std::unordered_map<std::string, std::int64_t> df{};
  for (const std::string& doc : documents) {
    std::vector<std::string> tokens =
        analyze_terms(doc, options_.lowercase,
                        options_.keep_punct, options_.min_ngram, options_.max_ngram);
    std::sort(tokens.begin(), tokens.end());
    tokens.erase(std::unique(tokens.begin(), tokens.end()), tokens.end());
    for (const std::string& token : tokens) {
      ++df[token];
    }
  }

  std::vector<std::pair<std::string, std::int64_t>> scored{};
  scored.reserve(df.size());
  for (const auto& kv : df) {
    if (kv.second >= options_.min_document_frequency) scored.emplace_back(kv.first, kv.second);
  }
  std::sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) {
    if (a.second != b.second) return a.second > b.second;
    return a.first < b.first;
  });

  if (scored.empty()) {
    return Status::invalid_argument("LexiconMapper::learn found no tokens passing min_document_frequency.");
  }

  const std::int64_t keep = std::min<std::int64_t>(options_.max_features, static_cast<std::int64_t>(scored.size()));
  vocabulary_.assign(static_cast<std::size_t>(keep), std::string{});
  document_frequency_.assign(static_cast<std::size_t>(keep), 0);
  for (std::int64_t i = 0; i < keep; ++i) {
    vocabulary_[static_cast<std::size_t>(i)] = std::move(scored[static_cast<std::size_t>(i)].first);
    document_frequency_[static_cast<std::size_t>(i)] = scored[static_cast<std::size_t>(i)].second;
  }

  learned_ = true;
  return Status::success();
}

StatusOr<FeatureBatch> LexiconMapper::map_dense(pluss::msharp::RangeView::Of<std::string> documents) const {
  if (!learned_) {
    return StatusOr<FeatureBatch>(Status::precondition("LexiconMapper::map_dense requires learn() first."));
  }
  if (documents.empty()) {
    return StatusOr<FeatureBatch>(Status::invalid_argument("LexiconMapper::map_dense requires at least one document."));
  }

  std::unordered_map<std::string, std::int64_t> vocab_index{};
  vocab_index.reserve(vocabulary_.size() * 2U + 1U);
  for (std::size_t i = 0; i < vocabulary_.size(); ++i) {
    vocab_index.emplace(vocabulary_[i], static_cast<std::int64_t>(i));
  }

  const std::int64_t rows = static_cast<std::int64_t>(documents.size());
  const std::int64_t cols = static_cast<std::int64_t>(vocabulary_.size());
  std::vector<double> matrix(static_cast<std::size_t>(rows * cols), 0.0);
  for (std::int64_t r = 0; r < rows; ++r) {
    const std::vector<std::string> tokens =
        analyze_terms(documents[static_cast<std::size_t>(r)],
                      options_.lowercase,
                        options_.keep_punct,
                      options_.min_ngram,
                      options_.max_ngram);
    for (const std::string& tok : tokens) {
      const auto it = vocab_index.find(tok);
      if (it == vocab_index.end()) continue;
      const std::size_t pos = static_cast<std::size_t>(r * cols + it->second);
      if (options_.binary) {
        matrix[pos] = 1.0;
      } else {
        matrix[pos] += 1.0;
      }
    }
  }

  return StatusOr<FeatureBatch>(FeatureBatch{
      .x = pluss::make_series(std::move(matrix), {rows, cols}, f64_cpu()),
      .layout = ObservationLayout::Rows,
  });
}

StatusOr<SparseTripletMatrix> LexiconMapper::map_sparse(pluss::msharp::RangeView::Of<std::string> documents) const {
  if (!learned_) {
    return StatusOr<SparseTripletMatrix>(Status::precondition("LexiconMapper::map_sparse requires learn() first."));
  }
  if (documents.empty()) {
    return StatusOr<SparseTripletMatrix>(Status::invalid_argument("LexiconMapper::map_sparse requires at least one document."));
  }

  std::unordered_map<std::string, std::int64_t> vocab_index{};
  vocab_index.reserve(vocabulary_.size() * 2U + 1U);
  for (std::size_t i = 0; i < vocabulary_.size(); ++i) {
    vocab_index.emplace(vocabulary_[i], static_cast<std::int64_t>(i));
  }

  std::vector<std::int64_t> row_idx{};
  std::vector<std::int64_t> col_idx{};
  std::vector<double> values{};

  const std::int64_t rows = static_cast<std::int64_t>(documents.size());
  const std::int64_t cols = static_cast<std::int64_t>(vocabulary_.size());

  for (std::int64_t r = 0; r < rows; ++r) {
    std::unordered_map<std::int64_t, double> local{};
    const std::vector<std::string> tokens =
        analyze_terms(documents[static_cast<std::size_t>(r)],
                      options_.lowercase,
                        options_.keep_punct,
                      options_.min_ngram,
                      options_.max_ngram);
    for (const std::string& tok : tokens) {
      const auto it = vocab_index.find(tok);
      if (it == vocab_index.end()) continue;
      if (options_.binary) {
        local[it->second] = 1.0;
      } else {
        local[it->second] += 1.0;
      }
    }
    std::vector<std::pair<std::int64_t, double>> sorted{};
    sorted.reserve(local.size());
    for (const auto& kv : local) sorted.push_back(kv);
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    for (const auto& kv : sorted) {
      row_idx.push_back(r);
      col_idx.push_back(kv.first);
      values.push_back(kv.second);
    }
  }

  return StatusOr<SparseTripletMatrix>(SparseTripletMatrix{
      .row_indices = pluss::make_series(row_idx, {static_cast<std::int64_t>(row_idx.size())}, i64_cpu()),
      .column_indices = pluss::make_series(col_idx, {static_cast<std::int64_t>(col_idx.size())}, i64_cpu()),
      .values = pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, f64_cpu()),
      .rows = rows,
      .cols = cols,
  });
}

StatusOr<FeatureBatch> LexiconMapper::map_tfidf_dense(pluss::msharp::RangeView::Of<std::string> documents,
                                                       const TfIdfMapOptions& options) const {
  if (!learned_) {
    return StatusOr<FeatureBatch>(Status::precondition("LexiconMapper::map_tfidf_dense requires learn() first."));
  }
  if (documents.empty()) {
    return StatusOr<FeatureBatch>(Status::invalid_argument("LexiconMapper::map_tfidf_dense requires at least one document."));
  }

  StatusOr<FeatureBatch> base_or = map_dense(documents);
  if (!base_or.ok()) return StatusOr<FeatureBatch>(base_or.status());
  FeatureBatch base = base_or.move_value();
  const std::int64_t rows = base.sample_count();
  const std::int64_t cols = base.feature_count();
  std::vector<double> x = base.x.to(pluss::pCPU, ScalarType::Float64).contiguous().to_vec<double>();

  const double n_docs = static_cast<double>(std::max<std::int64_t>(learned_document_count_, 1));
  std::vector<double> idf(static_cast<std::size_t>(cols), 1.0);
  for (std::int64_t c = 0; c < cols; ++c) {
    const double df = static_cast<double>(document_frequency_[static_cast<std::size_t>(c)]);
    if (options.smooth_idf) {
      idf[static_cast<std::size_t>(c)] = std::log((1.0 + n_docs) / (1.0 + df)) + 1.0;
    } else {
      idf[static_cast<std::size_t>(c)] = std::log((n_docs + 1e-12) / (df + 1e-12)) + 1.0;
    }
  }

  for (std::int64_t r = 0; r < rows; ++r) {
    double l2 = 0.0;
    const std::size_t base_idx = static_cast<std::size_t>(r * cols);
    for (std::int64_t c = 0; c < cols; ++c) {
      double tf = x[base_idx + static_cast<std::size_t>(c)];
      if (options.sublinear_tf && tf > 0.0) tf = 1.0 + std::log(tf);
      const double value = tf * idf[static_cast<std::size_t>(c)];
      x[base_idx + static_cast<std::size_t>(c)] = value;
      l2 += value * value;
    }
    if (options.l2_normalize && l2 > 0.0) {
      const double denom = std::sqrt(l2);
      for (std::int64_t c = 0; c < cols; ++c) {
        x[base_idx + static_cast<std::size_t>(c)] /= denom;
      }
    }
  }

  return StatusOr<FeatureBatch>(FeatureBatch{
      .x = pluss::make_series(std::move(x), {rows, cols}, f64_cpu()),
      .layout = ObservationLayout::Rows,
  });
}

StatusOr<SparseTripletMatrix> LexiconMapper::map_tfidf_sparse(pluss::msharp::RangeView::Of<std::string> documents,
                                                               const TfIdfMapOptions& options) const {
  if (!learned_) {
    return StatusOr<SparseTripletMatrix>(Status::precondition("LexiconMapper::map_tfidf_sparse requires learn() first."));
  }
  if (documents.empty()) {
    return StatusOr<SparseTripletMatrix>(Status::invalid_argument("LexiconMapper::map_tfidf_sparse requires at least one document."));
  }

  std::unordered_map<std::string, std::int64_t> vocab_index{};
  vocab_index.reserve(vocabulary_.size() * 2U + 1U);
  for (std::size_t i = 0; i < vocabulary_.size(); ++i) {
    vocab_index.emplace(vocabulary_[i], static_cast<std::int64_t>(i));
  }
  const std::int64_t rows = static_cast<std::int64_t>(documents.size());
  const std::int64_t cols = static_cast<std::int64_t>(vocabulary_.size());

  const double n_docs = static_cast<double>(std::max<std::int64_t>(learned_document_count_, 1));
  std::vector<double> idf(static_cast<std::size_t>(cols), 1.0);
  for (std::int64_t c = 0; c < cols; ++c) {
    const double df = static_cast<double>(document_frequency_[static_cast<std::size_t>(c)]);
    if (options.smooth_idf) {
      idf[static_cast<std::size_t>(c)] = std::log((1.0 + n_docs) / (1.0 + df)) + 1.0;
    } else {
      idf[static_cast<std::size_t>(c)] = std::log((n_docs + 1e-12) / (df + 1e-12)) + 1.0;
    }
  }

  std::vector<std::int64_t> row_idx{};
  std::vector<std::int64_t> col_idx{};
  std::vector<double> values{};
  for (std::int64_t r = 0; r < rows; ++r) {
    std::unordered_map<std::int64_t, double> local{};
    const std::vector<std::string> tokens =
        analyze_terms(documents[static_cast<std::size_t>(r)],
                      options_.lowercase,
                        options_.keep_punct,
                      options_.min_ngram,
                      options_.max_ngram);
    for (const std::string& tok : tokens) {
      const auto it = vocab_index.find(tok);
      if (it == vocab_index.end()) continue;
      if (options_.binary) {
        local[it->second] = 1.0;
      } else {
        local[it->second] += 1.0;
      }
    }

    double l2 = 0.0;
    std::vector<std::pair<std::int64_t, double>> sorted{};
    sorted.reserve(local.size());
    for (const auto& kv : local) {
      double tf = kv.second;
      if (options.sublinear_tf && tf > 0.0) tf = 1.0 + std::log(tf);
      const double value = tf * idf[static_cast<std::size_t>(kv.first)];
      sorted.emplace_back(kv.first, value);
      l2 += value * value;
    }
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    const double denom = (options.l2_normalize && l2 > 0.0) ? std::sqrt(l2) : 1.0;
    for (const auto& kv : sorted) {
      row_idx.push_back(r);
      col_idx.push_back(kv.first);
      values.push_back(kv.second / denom);
    }
  }

  return StatusOr<SparseTripletMatrix>(SparseTripletMatrix{
      .row_indices = pluss::make_series(row_idx, {static_cast<std::int64_t>(row_idx.size())}, i64_cpu()),
      .column_indices = pluss::make_series(col_idx, {static_cast<std::int64_t>(col_idx.size())}, i64_cpu()),
      .values = pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, f64_cpu()),
      .rows = rows,
      .cols = cols,
  });
}

HashMapper::HashMapper(HashMapOptions options) : options_(options) {}

StatusOr<FeatureBatch> HashMapper::map_dense(pluss::msharp::RangeView::Of<std::string> documents) const {
  if (documents.empty()) {
    return StatusOr<FeatureBatch>(Status::invalid_argument("HashMapper::map_dense requires at least one document."));
  }
  if (options_.n_features <= 0) {
    return StatusOr<FeatureBatch>(Status::invalid_argument("HashMapOptions.n_features must be > 0."));
  }
  if (const auto status =
          validate_ngram_range(options_.min_ngram, options_.max_ngram, "HashMapper::map_dense");
      !status.ok()) {
    return StatusOr<FeatureBatch>(status);
  }

  const std::int64_t rows = static_cast<std::int64_t>(documents.size());
  const std::int64_t cols = options_.n_features;
  std::vector<double> matrix(static_cast<std::size_t>(rows * cols), 0.0);
  for (std::int64_t r = 0; r < rows; ++r) {
    const auto terms =
        analyze_terms(documents[static_cast<std::size_t>(r)],
                      options_.lowercase,
                        options_.keep_punct,
                      options_.min_ngram,
                      options_.max_ngram);
    for (const auto& term : terms) {
      const auto hash = stable_hash64(term);
      const auto col = static_cast<std::int64_t>(hash % static_cast<std::uint64_t>(cols));
      const auto sign = options_.alternate_sign && ((hash >> 63U) != 0U) ? -1.0 : 1.0;
      const auto pos = static_cast<std::size_t>(r * cols + col);
      if (options_.binary) {
        matrix[pos] = sign;
      } else {
        matrix[pos] += sign;
      }
    }
  }
  return StatusOr<FeatureBatch>(FeatureBatch{
      .x = pluss::make_series(std::move(matrix), {rows, cols}, f64_cpu()),
      .layout = ObservationLayout::Rows,
  });
}

StatusOr<SparseTripletMatrix> HashMapper::map_sparse(pluss::msharp::RangeView::Of<std::string> documents) const {
  if (documents.empty()) {
    return StatusOr<SparseTripletMatrix>(Status::invalid_argument("HashMapper::map_sparse requires at least one document."));
  }
  if (options_.n_features <= 0) {
    return StatusOr<SparseTripletMatrix>(Status::invalid_argument("HashMapOptions.n_features must be > 0."));
  }
  if (const auto status =
          validate_ngram_range(options_.min_ngram, options_.max_ngram, "HashMapper::map_sparse");
      !status.ok()) {
    return StatusOr<SparseTripletMatrix>(status);
  }

  const std::int64_t rows = static_cast<std::int64_t>(documents.size());
  const std::int64_t cols = options_.n_features;
  std::vector<std::int64_t> row_idx{};
  std::vector<std::int64_t> col_idx{};
  std::vector<double> values{};

  for (std::int64_t r = 0; r < rows; ++r) {
    std::unordered_map<std::int64_t, double> local{};
    const auto terms =
        analyze_terms(documents[static_cast<std::size_t>(r)],
                      options_.lowercase,
                        options_.keep_punct,
                      options_.min_ngram,
                      options_.max_ngram);
    for (const auto& term : terms) {
      const auto hash = stable_hash64(term);
      const auto col = static_cast<std::int64_t>(hash % static_cast<std::uint64_t>(cols));
      const auto sign = options_.alternate_sign && ((hash >> 63U) != 0U) ? -1.0 : 1.0;
      if (options_.binary) {
        local[col] = sign;
      } else {
        local[col] += sign;
      }
    }
    std::vector<std::pair<std::int64_t, double>> sorted{};
    sorted.reserve(local.size());
    for (const auto& kv : local) sorted.push_back(kv);
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (const auto& kv : sorted) {
      row_idx.push_back(r);
      col_idx.push_back(kv.first);
      values.push_back(kv.second);
    }
  }

  return StatusOr<SparseTripletMatrix>(SparseTripletMatrix{
      .row_indices = pluss::make_series(row_idx, {static_cast<std::int64_t>(row_idx.size())}, i64_cpu()),
      .column_indices = pluss::make_series(col_idx, {static_cast<std::int64_t>(col_idx.size())}, i64_cpu()),
      .values = pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, f64_cpu()),
      .rows = rows,
      .cols = cols,
  });
}

} // namespace pluss::ai::nodej
