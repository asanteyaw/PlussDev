#include "pluss/ai/nodej/text.h"

#include "pluss/codem/series_factory.h"

#include <algorithm>
#include <cctype>
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
  std::string out = token;
  for (char& ch : out) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return out;
}

[[nodiscard]] std::vector<std::string> tokenize(const std::string& text, bool lowercase) {
  std::vector<std::string> tokens{};
  std::string cur{};
  cur.reserve(32);
  for (char ch : text) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch) || ch == '_') {
      cur.push_back(ch);
      continue;
    }
    if (!cur.empty()) {
      tokens.push_back(normalized_token(cur, lowercase));
      cur.clear();
    }
  }
  if (!cur.empty()) {
    tokens.push_back(normalized_token(cur, lowercase));
  }
  return tokens;
}

} // namespace

LexiconMapper::LexiconMapper(LexiconMapOptions options) : options_(options) {}

Status LexiconMapper::learn(pluss::msharp::RangeView::Of<std::string> documents) {
  if (documents.empty()) return Status::invalid_argument("LexiconMapper::learn requires at least one document.");
  if (options_.max_features <= 0) {
    return Status::invalid_argument("LexiconMapOptions.max_features must be > 0.");
  }
  if (options_.min_document_frequency <= 0) {
    return Status::invalid_argument("LexiconMapOptions.min_document_frequency must be > 0.");
  }

  learned_document_count_ = static_cast<std::int64_t>(documents.size());
  std::unordered_map<std::string, std::int64_t> df{};
  for (const std::string& doc : documents) {
    std::vector<std::string> tokens = tokenize(doc, options_.lowercase);
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
    const std::vector<std::string> tokens = tokenize(documents[static_cast<std::size_t>(r)], options_.lowercase);
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
    const std::vector<std::string> tokens = tokenize(documents[static_cast<std::size_t>(r)], options_.lowercase);
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
    const std::vector<std::string> tokens = tokenize(documents[static_cast<std::size_t>(r)], options_.lowercase);
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

} // namespace pluss::ai::nodej
