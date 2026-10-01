#pragma once

#include "pluss/ai/nodej/dataset.h"

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

namespace pluss::ai::nodej {

struct LexiconMapOptions {
  bool lowercase = true;
  std::int64_t max_features = 4096;
  std::int64_t min_document_frequency = 1;
  bool binary = false;
  std::int64_t min_ngram = 1;
  std::int64_t max_ngram = 1;
};

struct TfIdfMapOptions {
  bool smooth_idf = true;
  bool sublinear_tf = false;
  bool l2_normalize = true;
};

struct HashMapOptions {
  bool lowercase = true;
  std::int64_t n_features = 1 << 15;
  bool binary = false;
  bool alternate_sign = false;
  std::int64_t min_ngram = 1;
  std::int64_t max_ngram = 1;
};

struct LexiconMapperState {
  LexiconMapOptions options{};
  bool learned = false;
  std::int64_t learned_document_count = 0;
  std::vector<std::string> vocabulary{};
  std::vector<std::int64_t> document_frequency{};
};

struct SparseTripletMatrix {
  Series row_indices = pluss::series::undefined();
  Series column_indices = pluss::series::undefined();
  Series values = pluss::series::undefined();
  std::int64_t rows = 0;
  std::int64_t cols = 0;
};

class LexiconMapper {
public:
  explicit LexiconMapper(LexiconMapOptions options = {});

  [[nodiscard]] bool learned() const noexcept { return learned_; }
  [[nodiscard]] std::int64_t feature_count() const noexcept { return static_cast<std::int64_t>(vocabulary_.size()); }

  Status learn(pluss::msharp::RangeView::Of<std::string> documents);
  inline Status learn(std::initializer_list<std::string> documents) {
    return learn(pluss::msharp::RangeView::Of<std::string>(documents));
  }

  StatusOr<FeatureBatch> map_dense(pluss::msharp::RangeView::Of<std::string> documents) const;
  inline StatusOr<FeatureBatch> map_dense(std::initializer_list<std::string> documents) const {
    return map_dense(pluss::msharp::RangeView::Of<std::string>(documents));
  }

  StatusOr<SparseTripletMatrix> map_sparse(pluss::msharp::RangeView::Of<std::string> documents) const;
  inline StatusOr<SparseTripletMatrix> map_sparse(std::initializer_list<std::string> documents) const {
    return map_sparse(pluss::msharp::RangeView::Of<std::string>(documents));
  }

  StatusOr<FeatureBatch> map_tfidf_dense(pluss::msharp::RangeView::Of<std::string> documents,
                                         const TfIdfMapOptions& options = {}) const;
  inline StatusOr<FeatureBatch> map_tfidf_dense(std::initializer_list<std::string> documents,
                                                const TfIdfMapOptions& options = {}) const {
    return map_tfidf_dense(pluss::msharp::RangeView::Of<std::string>(documents), options);
  }

  StatusOr<SparseTripletMatrix> map_tfidf_sparse(pluss::msharp::RangeView::Of<std::string> documents,
                                                 const TfIdfMapOptions& options = {}) const;
  inline StatusOr<SparseTripletMatrix> map_tfidf_sparse(std::initializer_list<std::string> documents,
                                                        const TfIdfMapOptions& options = {}) const {
    return map_tfidf_sparse(pluss::msharp::RangeView::Of<std::string>(documents), options);
  }

  [[nodiscard]] const std::vector<std::string>& vocabulary() const noexcept { return vocabulary_; }
  [[nodiscard]] LexiconMapperState dump_state() const;
  Status load_state(const LexiconMapperState& state);

private:
  LexiconMapOptions options_{};
  bool learned_{false};
  std::int64_t learned_document_count_{0};
  std::vector<std::string> vocabulary_{};
  std::vector<std::int64_t> document_frequency_{};
};

class HashMapper {
public:
  explicit HashMapper(HashMapOptions options = {});

  [[nodiscard]] const HashMapOptions& options() const noexcept { return options_; }

  StatusOr<FeatureBatch> map_dense(pluss::msharp::RangeView::Of<std::string> documents) const;
  inline StatusOr<FeatureBatch> map_dense(std::initializer_list<std::string> documents) const {
    return map_dense(pluss::msharp::RangeView::Of<std::string>(documents));
  }

  StatusOr<SparseTripletMatrix> map_sparse(pluss::msharp::RangeView::Of<std::string> documents) const;
  inline StatusOr<SparseTripletMatrix> map_sparse(std::initializer_list<std::string> documents) const {
    return map_sparse(pluss::msharp::RangeView::Of<std::string>(documents));
  }

private:
  HashMapOptions options_{};
};

} // namespace pluss::ai::nodej
