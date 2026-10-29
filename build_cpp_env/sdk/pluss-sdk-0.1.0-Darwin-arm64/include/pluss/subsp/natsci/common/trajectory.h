#pragma once

#include "pluss/subsp/natsci/common/record.h"

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace pluss::subsp::natsci::common {

template <DimensionType CoordinateDimension> class Trajectory final {
public:
  Trajectory(SeriesQuantity<CoordinateDimension> coordinate, StateVector states,
             bool require_ordered = true)
      : coordinate_(std::move(coordinate)), states_(std::move(states)),
        ordered_(require_ordered) {
    validate_();
  }

  [[nodiscard]] const SeriesQuantity<CoordinateDimension> &
  coordinate() const noexcept {
    return coordinate_;
  }
  [[nodiscard]] const StateVector &state() const noexcept { return states_; }
  [[nodiscard]] bool ordered() const noexcept { return ordered_; }
  [[nodiscard]] std::int64_t steps() const noexcept {
    return coordinate_.si_value().size(-1);
  }

private:
  void validate_() const {
    const auto &coordinate = coordinate_.si_value();
    detail::require_numeric(coordinate, "natsci::common::Trajectory coordinate",
                            false);
    if (coordinate.dim() < 1)
      throw std::invalid_argument(
          "natsci::common::Trajectory: coordinate must have rank at least one");

    const auto coordinate_shape = coordinate.sizes();
    for (const auto &block : states_.layout().blocks()) {
      if (block.platform != coordinate.platform())
        throw std::invalid_argument("natsci::common::Trajectory: coordinate "
                                    "and states must share a platform");
      if (block.sizes.size() < coordinate_shape.size())
        throw std::invalid_argument("natsci::common::Trajectory: each state "
                                    "must retain the coordinate axes");
      for (std::size_t axis = 0; axis < coordinate_shape.size(); ++axis) {
        if (block.sizes[axis] != coordinate_shape[axis])
          throw std::invalid_argument("natsci::common::Trajectory: state shape "
                                      "does not begin with coordinate shape");
      }
    }

    if (ordered_ && coordinate.size(-1) > 1 &&
        !coordinate.diff(1, -1).gt(0.0).all().template item<bool>()) {
      throw std::invalid_argument(
          "natsci::common::Trajectory: coordinate must be strictly increasing");
    }
  }

  SeriesQuantity<CoordinateDimension> coordinate_;
  StateVector states_;
  bool ordered_;
};

template <DimensionType CoordinateDimension>
[[nodiscard]] Trajectory<CoordinateDimension>
path(SeriesQuantity<CoordinateDimension> coordinate, StateVector states,
     bool require_ordered = true) {
  return Trajectory<CoordinateDimension>(std::move(coordinate),
                                         std::move(states), require_ordered);
}

} // namespace pluss::subsp::natsci::common
