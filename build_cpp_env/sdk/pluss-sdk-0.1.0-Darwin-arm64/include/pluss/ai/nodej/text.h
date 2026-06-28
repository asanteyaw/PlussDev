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
};

struct TfIdfMapOptions {
  bool smooth_idf = true;
  bool sublinear_tf = false;
  bool l2_normalize = true;
};

struct SparseTripletMatrix {
  Series row_indices = Series::undefined();
  Series column_indices = Series::undefined();
  Series values = Series::undefined();
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

private:
  LexiconMapOptions options_{};
  bool learned_{false};
  std::int64_t learned_document_count_{0};
  std::vector<std::string> vocabulary_{};
  std::vector<std::int64_t> document_frequency_{};
};

} // namespace pluss::ai::nodej
