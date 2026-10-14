#pragma once

#include "pluss/subsp/math/covariance.h"
#include "pluss/subsp/natsci/common/quantity.h"

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace pluss::subsp::natsci::common {

enum class CovarianceCheck {
  Symmetric,
  PositiveSemidefinite,
  PositiveDefinite
};

// A homogeneous vector of measured values and its full covariance matrix.
// Batch axes are preserved: values [..., n], covariance [..., n, n].
template <DimensionType D> class CorrelatedMeasurement final {
public:
  using CovarianceDimension = DimensionPower<D, 2>;

  CorrelatedMeasurement(
      SeriesQuantity<D> values, SeriesQuantity<CovarianceDimension> covariance,
      CovarianceCheck check = CovarianceCheck::PositiveSemidefinite)
      : values_(std::move(values)), covariance_(std::move(covariance)),
        check_(check) {
    validate_();
  }

  [[nodiscard]] const SeriesQuantity<D> &values() const noexcept {
    return values_;
  }
  [[nodiscard]] const SeriesQuantity<CovarianceDimension> &
  covariance() const noexcept {
    return covariance_;
  }
  [[nodiscard]] CovarianceCheck check() const noexcept { return check_; }

  // Applies y = A*x and C_y = A*C_x*A^T with batched matmul kernels.
  [[nodiscard]] CorrelatedMeasurement map(const Series &matrix) const {
    const auto &values = values_.si_value();
    const auto &covariance = covariance_.si_value();
    if (!matrix.defined() || !matrix.dtype().is_floating())
      throw std::invalid_argument("natsci::common::CorrelatedMeasurement::map: "
                                  "expected a real floating matrix");
    if (matrix.dim() < 2 || matrix.size(-1) != values.size(-1))
      throw std::invalid_argument("natsci::common::CorrelatedMeasurement::map: "
                                  "matrix input width mismatch");
    if (matrix.scalar_type() != values.scalar_type())
      throw std::invalid_argument(
          "natsci::common::CorrelatedMeasurement::map: dtype mismatch");
    if (matrix.platform() != values.platform())
      throw std::invalid_argument(
          "natsci::common::CorrelatedMeasurement::map: platform mismatch");

    Series mapped_values =
        matrix.linearalg.matmul(values.unsqueeze(-1)).squeeze(-1);
    Series mapped_covariance = matrix.linearalg.matmul(covariance)
                                   .linearalg.matmul(matrix.transpose(-2, -1));
    return CorrelatedMeasurement(
        SeriesQuantity<D>::from_si(std::move(mapped_values)),
        SeriesQuantity<CovarianceDimension>::from_si(
            std::move(mapped_covariance)),
        CovarianceCheck::PositiveSemidefinite);
  }

private:
  void validate_() const {
    const auto &values = values_.si_value();
    const auto &covariance = covariance_.si_value();
    if (!values.defined())
      throw std::invalid_argument("natsci::common::CorrelatedMeasurement: "
                                  "expected defined Series values");
    if (!values.dtype().is_floating())
      throw std::invalid_argument("natsci::common::CorrelatedMeasurement: "
                                  "expected real floating values");
    pluss::subsp::math::cov_check(covariance,
                                  "natsci::common::CorrelatedMeasurement");
    if (values.dim() < 1 || covariance.dim() != values.dim() + 1)
      throw std::invalid_argument(
          "natsci::common::CorrelatedMeasurement: expected values [..., n] and "
          "covariance [..., n, n]");
    if (values.scalar_type() != covariance.scalar_type())
      throw std::invalid_argument(
          "natsci::common::CorrelatedMeasurement: dtype mismatch");
    if (values.platform() != covariance.platform())
      throw std::invalid_argument(
          "natsci::common::CorrelatedMeasurement: platform mismatch");

    for (std::int64_t axis = 0; axis < values.dim() - 1; ++axis) {
      if (values.size(axis) != covariance.size(axis))
        throw std::invalid_argument(
            "natsci::common::CorrelatedMeasurement: batch shape mismatch");
    }
    const auto width = values.size(-1);
    if (width == 0)
      throw std::invalid_argument("natsci::common::CorrelatedMeasurement: "
                                  "covariance width must be positive");
    if (covariance.size(-2) != width || covariance.size(-1) != width)
      throw std::invalid_argument(
          "natsci::common::CorrelatedMeasurement: covariance width mismatch");
    if (!values.isfinite().all().template item<bool>())
      throw std::invalid_argument(
          "natsci::common::CorrelatedMeasurement: values must be finite");
    if (!pluss::subsp::math::cov_is_sym(covariance))
      throw std::invalid_argument("natsci::common::CorrelatedMeasurement: "
                                  "covariance must be symmetric");
    if (!pluss::subsp::math::cov_diag(covariance)
             .ge(0.0)
             .all()
             .template item<bool>())
      throw std::invalid_argument("natsci::common::CorrelatedMeasurement: "
                                  "covariance diagonal must be non-negative");
    if (check_ != CovarianceCheck::Symmetric) {
      try {
        const Series checked =
            check_ == CovarianceCheck::PositiveSemidefinite
                ? pluss::subsp::math::cov_jitter(
                      covariance,
                      covariance.scalar_type() == ScalarType::Float64 ? 1e-12
                                                                      : 1e-5)
                : covariance;
        (void)checked.linearalg.cholesky();
      } catch (const std::exception &) {
        throw std::invalid_argument(
            check_ == CovarianceCheck::PositiveSemidefinite
                ? "natsci::common::CorrelatedMeasurement: covariance must be "
                  "positive semidefinite"
                : "natsci::common::CorrelatedMeasurement: covariance must be "
                  "positive definite");
      }
    }
  }

  SeriesQuantity<D> values_;
  SeriesQuantity<CovarianceDimension> covariance_;
  CovarianceCheck check_;
};

} // namespace pluss::subsp::natsci::common
