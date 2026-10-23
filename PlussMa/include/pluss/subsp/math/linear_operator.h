#pragma once

#include "pluss/codem/series_factory.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::math::linear_operator {

// A differentiable COO linear map. Indices describe immutable structure;
// values remain a Series so platform placement, batching of inputs, and
// reverse-mode derivatives use the ordinary Series kernels.
class CooMap final {
public:
  CooMap(std::int64_t rows, std::int64_t columns, Series row_index,
         Series column_index, Series values)
      : rows_(rows), columns_(columns), row_index_(std::move(row_index)),
        column_index_(std::move(column_index)), values_(std::move(values)) {
    constexpr const char *operation = "math::linear_operator::CooMap";
    if (rows_ <= 0 || columns_ <= 0)
      throw std::invalid_argument(std::string(operation) +
                                  ": dimensions must be positive");
    if (!row_index_.defined() || !column_index_.defined() ||
        !values_.defined() || row_index_.dim() != 1 ||
        column_index_.dim() != 1 || values_.dim() != 1 ||
        row_index_.scalar_type() != ScalarType::Int64 ||
        column_index_.scalar_type() != ScalarType::Int64 ||
        row_index_.numel() != column_index_.numel() ||
        row_index_.numel() != values_.numel())
      throw std::invalid_argument(
          std::string(operation) +
          ": row, column, and value arrays must be equally sized vectors; "
          "indices must be Int64");
    if (!values_.dtype().is_floating() && !values_.dtype().is_complex())
      throw std::invalid_argument(std::string(operation) +
                                  ": values must be floating or complex");
    if (values_.numel() == 0)
      throw std::invalid_argument(std::string(operation) +
                                  ": at least one nonzero is required");
    const bool finite = values_.dtype().is_complex()
                            ? values_.real().isfinite().all().item<bool>() &&
                                  values_.imag().isfinite().all().item<bool>()
                            : values_.isfinite().all().item<bool>();
    if (!finite)
      throw std::invalid_argument(std::string(operation) +
                                  ": values must be finite");
    const Series cpu_rows = row_index_.to(Platform::CPU);
    const Series cpu_columns = column_index_.to(Platform::CPU);
    if (cpu_rows.min().item<std::int64_t>() < 0 ||
        cpu_rows.max().item<std::int64_t>() >= rows_ ||
        cpu_columns.min().item<std::int64_t>() < 0 ||
        cpu_columns.max().item<std::int64_t>() >= columns_)
      throw std::out_of_range(std::string(operation) +
                              ": an index lies outside the map dimensions");
    row_index_ = row_index_.to(values_.platform());
    column_index_ = column_index_.to(values_.platform());
  }

  [[nodiscard]] std::int64_t rows() const noexcept { return rows_; }
  [[nodiscard]] std::int64_t columns() const noexcept { return columns_; }
  [[nodiscard]] std::int64_t nnz() const noexcept { return values_.numel(); }
  [[nodiscard]] const Series &row_index() const noexcept { return row_index_; }
  [[nodiscard]] const Series &column_index() const noexcept {
    return column_index_;
  }
  [[nodiscard]] const Series &values() const noexcept { return values_; }

  [[nodiscard]] Series apply(const Series &input) const {
    constexpr const char *operation = "math::linear_operator::CooMap::apply";
    if (!input.defined() || input.dim() < 1 || input.size(-1) != columns_)
      throw std::invalid_argument(std::string(operation) +
                                  ": input must have shape [..., columns]");
    if ((!input.dtype().is_floating() && !input.dtype().is_complex()) ||
        input.platform() != values_.platform())
      throw std::invalid_argument(
          std::string(operation) +
          ": input and values must be numeric and share a platform");

    std::vector<std::int64_t> index_shape(static_cast<std::size_t>(input.dim()),
                                          1);
    index_shape.back() = nnz();
    std::vector<std::int64_t> gather_shape(input.sizes().begin(),
                                           input.sizes().end());
    gather_shape.back() = nnz();
    const Series columns =
        column_index_.reshape(index_shape).expand(gather_shape, true);
    const Series rows =
        row_index_.reshape(index_shape).expand(gather_shape, true);
    const Series weights =
        values_.reshape(index_shape).expand(gather_shape, true);
    const Series products = input.gather(-1, columns).mul(weights);
    std::vector<std::int64_t> output_shape(input.sizes().begin(),
                                           input.sizes().end());
    output_shape.back() = rows_;
    const Series output =
        Series::zeros(output_shape, products.scalar_type(), input.platform());
    return output.scatter_add(-1, rows, products);
  }

  [[nodiscard]] Series apply_columns(const Series &input) const {
    if (!input.defined() || input.dim() < 2 || input.size(-2) != columns_)
      throw std::invalid_argument(
          "math::linear_operator::CooMap::apply_columns: input must have "
          "shape [..., columns, lanes]");
    return apply(input.transpose(-2, -1)).transpose(-2, -1);
  }

  [[nodiscard]] CooMap adjoint() const {
    return CooMap(columns_, rows_, column_index_, row_index_, values_.conj());
  }

private:
  std::int64_t rows_;
  std::int64_t columns_;
  Series row_index_;
  Series column_index_;
  Series values_;
};

[[nodiscard]] inline CooMap coo(std::int64_t rows, std::int64_t columns,
                                Series row_index, Series column_index,
                                Series values) {
  return CooMap(rows, columns, std::move(row_index), std::move(column_index),
                std::move(values));
}

} // namespace pluss::subsp::math::linear_operator
