#pragma once

#include "pluss/subsp/natsci/common/quantity.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace pluss::subsp::natsci::common {

template <DimensionType D> struct Drift final {
  SeriesQuantity<D> absolute;
  SeriesQuantity<Dimensionless> scaled;
};

// Normalize against an explicit physical scale. An explicit scale avoids
// hiding a unit-dependent epsilon inside a dimensioned calculation.
template <DimensionType D>
[[nodiscard]] inline Drift<D> drift(const SeriesQuantity<D> &reference,
                                    const SeriesQuantity<D> &current,
                                    ScalarQuantity<D> scale) {
  if (!(scale.si_value() > 0.0) || !std::isfinite(scale.si_value()))
    throw std::invalid_argument(
        "natsci::common::drift: scale must be finite and positive");

  const Series &lhs = reference.si_value();
  const Series &rhs = current.si_value();
  if (lhs.dim() != rhs.dim())
    throw std::invalid_argument("natsci::common::drift: shape mismatch");
  for (std::int64_t axis = 0; axis < lhs.dim(); ++axis)
    if (lhs.size(axis) != rhs.size(axis))
      throw std::invalid_argument("natsci::common::drift: shape mismatch");
  if (lhs.scalar_type() != rhs.scalar_type())
    throw std::invalid_argument("natsci::common::drift: dtype mismatch");
  if (lhs.platform() != rhs.platform())
    throw std::invalid_argument("natsci::common::drift: platform mismatch");

  const Series absolute = rhs.sub(lhs).abs();
  return {
      SeriesQuantity<D>::from_si(absolute),
      SeriesQuantity<Dimensionless>::from_si(absolute.div(scale.si_value()))};
}

} // namespace pluss::subsp::natsci::common
