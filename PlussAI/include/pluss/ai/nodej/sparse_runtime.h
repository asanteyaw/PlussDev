#pragma once

#include "pluss/ai/nodej/text.h"
#include "pluss/ai/nodej/options.h"
#include "pluss/ai/nodej/families.h"
#include "MSharp/RangeView.h"

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace pluss::ai::nodej {

struct SparseCsrMatrix {
  std::vector<std::int64_t> row_ptr{};
  std::vector<std::int64_t> col_ind{};
  std::vector<double> values{};
  std::int64_t rows = 0;
  std::int64_t cols = 0;
};

StatusOr<SparseTripletMatrix> make_sparse_triplet(std::int64_t rows,
                                                  std::int64_t cols,
                                                  pluss::msharp::RangeView::Int row_indices,
                                                  pluss::msharp::RangeView::Int column_indices,
                                                  pluss::msharp::RangeView::Of<double> values);
inline StatusOr<SparseTripletMatrix> make_sparse_triplet(
    std::int64_t rows,
    std::int64_t cols,
    std::initializer_list<std::int64_t> row_indices,
    std::initializer_list<std::int64_t> column_indices,
    std::initializer_list<double> values) {
  return make_sparse_triplet(rows,
                             cols,
                             pluss::msharp::RangeView::Int(row_indices),
                             pluss::msharp::RangeView::Int(column_indices),
                             pluss::msharp::RangeView::Of<double>(values));
}

StatusOr<SparseCsrMatrix> sparse_triplet_to_csr(const SparseTripletMatrix& sparse);

class SparseLinearBinaryClassifier final {
public:
  SparseLinearBinaryClassifier() = default;

  [[nodiscard]] bool trained() const noexcept { return trained_; }
  [[nodiscard]] std::int64_t feature_count() const noexcept { return feature_count_; }

  Status learn(const SparseTripletMatrix& x,
               const Series& y,
               const TrainOptions& options = {},
               const LinearFamilyOptions& linear_options = {});

  [[nodiscard]] StatusOr<Series> infer_scores(const SparseTripletMatrix& x) const;
  [[nodiscard]] StatusOr<Series> infer(const SparseTripletMatrix& x, const InferOptions& options = {}) const;
  [[nodiscard]] StatusOr<Series> assess(const SparseTripletMatrix& x,
                                        const Series& y,
                                        const AssessOptions& options = {}) const;

  [[nodiscard]] StatusOr<std::vector<Series>> dump_state() const;
  Status load_state(pluss::msharp::RangeView::Series state);

private:
  [[nodiscard]] StatusOr<std::vector<double>> infer_scores_cpu_(const SparseCsrMatrix& csr) const;

  bool trained_{false};
  std::int64_t feature_count_{0};
  std::vector<double> weights_{};
};

} // namespace pluss::ai::nodej
