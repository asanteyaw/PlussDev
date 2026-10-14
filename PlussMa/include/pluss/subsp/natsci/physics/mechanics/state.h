#pragma once

#include "pluss/subsp/natsci/common/quantity.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::physics::mechanics {

namespace common = ::pluss::subsp::natsci::common;

template <common::DimensionType CoordinateDimension>
using CoordinateRate =
    common::DimensionQuotient<CoordinateDimension, common::Time>;

template <common::DimensionType CoordinateDimension>
using CoordinateAcceleration =
    common::DimensionQuotient<CoordinateRate<CoordinateDimension>,
                              common::Time>;

template <common::DimensionType CoordinateDimension>
using CanonicalMomentum =
    common::DimensionQuotient<common::Action, CoordinateDimension>;

template <common::DimensionType CoordinateDimension>
using CanonicalForce =
    common::DimensionQuotient<CanonicalMomentum<CoordinateDimension>,
                              common::Time>;

template <common::DimensionType CoordinateDimension>
using GeneralizedMass = common::DimensionQuotient<
    common::Energy,
    common::DimensionPower<CoordinateRate<CoordinateDimension>, 2>>;

namespace detail {

inline void require_variational_value(const Series &value,
                                      const char *operation, const char *role) {
  if (!value.defined())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must be defined");
  if (!value.dtype().is_floating())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must have a real floating dtype");
  if (value.numel() == 0)
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must not be empty");
  if (!value.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must be finite");
}

inline bool same_variational_shape(const Series &lhs, const Series &rhs) {
  if (lhs.dim() != rhs.dim())
    return false;
  for (std::int64_t axis = 0; axis < lhs.dim(); ++axis)
    if (lhs.size(axis) != rhs.size(axis))
      return false;
  return true;
}

inline void require_variational_layout(const Series &coordinate,
                                       const Series &peer,
                                       const char *operation,
                                       const char *role) {
  if (!same_variational_shape(coordinate, peer))
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " shape must match coordinate shape");
  if (coordinate.scalar_type() != peer.scalar_type())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " dtype must match coordinate dtype");
  if (coordinate.platform() != peer.platform())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " platform must match coordinate platform");
}

inline void require_coordinate_axis(const Series &coordinate,
                                    const char *operation) {
  if (coordinate.dim() < 1 || coordinate.size(-1) <= 0)
    throw std::invalid_argument(std::string(operation) +
                                ": coordinate must have shape [..., dof] "
                                "with dof greater than zero");
}

inline std::vector<std::int64_t> batch_shape(const Series &coordinate) {
  return {coordinate.sizes().begin(), coordinate.sizes().end() - 1};
}

} // namespace detail

template <common::DimensionType CoordinateDimension> class TangentState final {
public:
  [[nodiscard]] const common::SeriesQuantity<CoordinateDimension> &
  coordinate() const noexcept {
    return coordinate_;
  }

  [[nodiscard]] const common::SeriesQuantity<
      CoordinateRate<CoordinateDimension>> &
  velocity() const noexcept {
    return velocity_;
  }

  [[nodiscard]] std::int64_t dof() const noexcept {
    return coordinate_.si_value().size(-1);
  }

  [[nodiscard]] std::vector<std::int64_t> batch_shape() const {
    return detail::batch_shape(coordinate_.si_value());
  }

private:
  TangentState(
      common::SeriesQuantity<CoordinateDimension> coordinate,
      common::SeriesQuantity<CoordinateRate<CoordinateDimension>> velocity)
      : coordinate_(std::move(coordinate)), velocity_(std::move(velocity)) {}

  common::SeriesQuantity<CoordinateDimension> coordinate_;
  common::SeriesQuantity<CoordinateRate<CoordinateDimension>> velocity_;

  template <common::DimensionType D>
  friend TangentState<D>
  make_tangent(const common::SeriesQuantity<D> &,
               const common::SeriesQuantity<CoordinateRate<D>> &);
};

template <common::DimensionType CoordinateDimension>
[[nodiscard]] inline TangentState<CoordinateDimension>
make_tangent(const common::SeriesQuantity<CoordinateDimension> &coordinate,
             const common::SeriesQuantity<CoordinateRate<CoordinateDimension>>
                 &velocity) {
  const Series &q = coordinate.si_value();
  const Series &v = velocity.si_value();
  detail::require_variational_value(q, "natsci::mechanics::make_tangent",
                                    "coordinate");
  detail::require_variational_value(v, "natsci::mechanics::make_tangent",
                                    "velocity");
  detail::require_coordinate_axis(q, "natsci::mechanics::make_tangent");
  detail::require_variational_layout(q, v, "natsci::mechanics::make_tangent",
                                     "velocity");
  return TangentState<CoordinateDimension>(coordinate, velocity);
}

template <common::DimensionType CoordinateDimension> class PhaseState final {
public:
  [[nodiscard]] const common::SeriesQuantity<CoordinateDimension> &
  coordinate() const noexcept {
    return coordinate_;
  }

  [[nodiscard]] const common::SeriesQuantity<
      CanonicalMomentum<CoordinateDimension>> &
  momentum() const noexcept {
    return momentum_;
  }

  [[nodiscard]] std::int64_t dof() const noexcept {
    return coordinate_.si_value().size(-1);
  }

  [[nodiscard]] std::vector<std::int64_t> batch_shape() const {
    return detail::batch_shape(coordinate_.si_value());
  }

private:
  PhaseState(
      common::SeriesQuantity<CoordinateDimension> coordinate,
      common::SeriesQuantity<CanonicalMomentum<CoordinateDimension>> momentum)
      : coordinate_(std::move(coordinate)), momentum_(std::move(momentum)) {}

  common::SeriesQuantity<CoordinateDimension> coordinate_;
  common::SeriesQuantity<CanonicalMomentum<CoordinateDimension>> momentum_;

  template <common::DimensionType D>
  friend PhaseState<D>
  make_phase(const common::SeriesQuantity<D> &,
             const common::SeriesQuantity<CanonicalMomentum<D>> &);
};

template <common::DimensionType CoordinateDimension>
[[nodiscard]] inline PhaseState<CoordinateDimension>
make_phase(const common::SeriesQuantity<CoordinateDimension> &coordinate,
           const common::SeriesQuantity<CanonicalMomentum<CoordinateDimension>>
               &momentum) {
  const Series &q = coordinate.si_value();
  const Series &p = momentum.si_value();
  detail::require_variational_value(q, "natsci::mechanics::make_phase",
                                    "coordinate");
  detail::require_variational_value(p, "natsci::mechanics::make_phase",
                                    "momentum");
  detail::require_coordinate_axis(q, "natsci::mechanics::make_phase");
  detail::require_variational_layout(q, p, "natsci::mechanics::make_phase",
                                     "momentum");
  return PhaseState<CoordinateDimension>(coordinate, momentum);
}

template <common::DimensionType CoordinateDimension> struct TangentRate final {
  common::SeriesQuantity<CoordinateRate<CoordinateDimension>> coordinate;
  common::SeriesQuantity<CoordinateAcceleration<CoordinateDimension>> velocity;
};

template <common::DimensionType CoordinateDimension> struct PhaseRate final {
  common::SeriesQuantity<CoordinateRate<CoordinateDimension>> coordinate;
  common::SeriesQuantity<CanonicalForce<CoordinateDimension>> momentum;
};

struct VariationalOptions final {
  // Retain derivative history so fields can participate in outer sensitivity
  // calculations. Disable both switches for one-shot simulation output.
  bool track_higher_order{true};
  bool keep_graph{true};
};

} // namespace pluss::subsp::natsci::physics::mechanics
