#pragma once

#include "pluss/subsp/natsci/common/quantity.h"

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::common::spatial {

class PeriodicGrid final {
public:
  [[nodiscard]] std::int64_t dim() const noexcept {
    return static_cast<std::int64_t>(shape_.size());
  }

  [[nodiscard]] const std::vector<std::int64_t> &shape() const noexcept {
    return shape_;
  }

  [[nodiscard]] ScalarQuantity<Length> spacing(std::int64_t axis) const {
    require_axis(axis, "PeriodicGrid::spacing");
    return ScalarQuantity<Length>::from_si(
        spacing_[static_cast<std::size_t>(axis)]);
  }

private:
  PeriodicGrid(std::vector<std::int64_t> shape, std::vector<double> spacing)
      : shape_(std::move(shape)), spacing_(std::move(spacing)) {}

  void require_axis(std::int64_t axis, const char *operation) const {
    if (axis < 0 || axis >= dim())
      throw std::out_of_range(std::string(operation) +
                              ": spatial axis is out of range");
  }

  std::vector<std::int64_t> shape_;
  std::vector<double> spacing_;

  friend PeriodicGrid make_periodic_grid(std::vector<std::int64_t>,
                                         std::vector<ScalarQuantity<Length>>);
  friend Series partial(const Series &, const PeriodicGrid &, std::int64_t);
  friend Series fwd_diff(const Series &, const PeriodicGrid &, std::int64_t);
  friend Series back_diff(const Series &, const PeriodicGrid &, std::int64_t);
  friend Series second_partial(const Series &, const PeriodicGrid &,
                               std::int64_t);
};

[[nodiscard]] inline PeriodicGrid
make_periodic_grid(std::vector<std::int64_t> shape,
                   std::vector<ScalarQuantity<Length>> spacing) {
  if (shape.empty() || shape.size() > 3)
    throw std::invalid_argument(
        "natsci::spatial::make_periodic_grid: expected one to three spatial "
        "axes");
  if (shape.size() != spacing.size())
    throw std::invalid_argument(
        "natsci::spatial::make_periodic_grid: shape and spacing rank mismatch");

  std::vector<double> spacing_si;
  spacing_si.reserve(spacing.size());
  for (std::size_t axis = 0; axis < shape.size(); ++axis) {
    if (shape[axis] < 3)
      throw std::invalid_argument(
          "natsci::spatial::make_periodic_grid: every extent must be at least "
          "three for a centered periodic stencil");
    const double value = spacing[axis].si_value();
    if (!(value > 0.0) || !std::isfinite(value))
      throw std::invalid_argument(
          "natsci::spatial::make_periodic_grid: spacing must be finite and "
          "positive");
    spacing_si.push_back(value);
  }
  return PeriodicGrid(std::move(shape), std::move(spacing_si));
}

namespace detail {

inline void require_numeric_field(const Series &value, const char *operation) {
  if (!value.defined())
    throw std::invalid_argument(std::string(operation) +
                                ": field must be defined");
  if (!value.dtype().is_floating() && !value.dtype().is_complex())
    throw std::invalid_argument(std::string(operation) +
                                ": field must have a floating or complex "
                                "dtype");
}

inline void require_scalar_layout(const Series &value, const PeriodicGrid &grid,
                                  const char *operation) {
  require_numeric_field(value, operation);
  if (value.dim() < grid.dim())
    throw std::invalid_argument(std::string(operation) +
                                ": field rank is smaller than grid rank");
  const std::int64_t first_spatial = value.dim() - grid.dim();
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    if (value.size(first_spatial + axis) !=
        grid.shape()[static_cast<std::size_t>(axis)])
      throw std::invalid_argument(std::string(operation) +
                                  ": trailing spatial shape does not match "
                                  "the grid");
  }
}

inline std::int64_t require_vector_layout(const Series &value,
                                          const PeriodicGrid &grid,
                                          const char *operation) {
  require_numeric_field(value, operation);
  if (value.dim() < grid.dim() + 1)
    throw std::invalid_argument(std::string(operation) +
                                ": vector field has no component axis");
  const std::int64_t component_axis = value.dim() - grid.dim() - 1;
  if (value.size(component_axis) != grid.dim())
    throw std::invalid_argument(std::string(operation) +
                                ": component count must equal grid rank");
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    if (value.size(component_axis + 1 + axis) !=
        grid.shape()[static_cast<std::size_t>(axis)])
      throw std::invalid_argument(std::string(operation) +
                                  ": trailing spatial shape does not match "
                                  "the grid");
  }
  return component_axis;
}

inline std::int64_t require_tensor_layout(const Series &value,
                                          const PeriodicGrid &grid,
                                          const char *operation) {
  require_numeric_field(value, operation);
  if (value.dim() < grid.dim() + 2)
    throw std::invalid_argument(std::string(operation) +
                                ": tensor field has fewer than two component "
                                "axes");
  const std::int64_t row_axis = value.dim() - grid.dim() - 2;
  if (value.size(row_axis) != grid.dim() ||
      value.size(row_axis + 1) != grid.dim())
    throw std::invalid_argument(std::string(operation) +
                                ": both tensor component counts must equal "
                                "grid rank");
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    if (value.size(row_axis + 2 + axis) !=
        grid.shape()[static_cast<std::size_t>(axis)])
      throw std::invalid_argument(std::string(operation) +
                                  ": trailing spatial shape does not match "
                                  "the grid");
  }
  return row_axis;
}

inline void require_axis(const PeriodicGrid &grid, std::int64_t axis,
                         const char *operation) {
  if (axis < 0 || axis >= grid.dim())
    throw std::out_of_range(std::string(operation) +
                            ": spatial axis is out of range");
}

inline Series stack_components(const std::vector<Series> &components,
                               std::int64_t axis) {
  Series result = components.front().unsqueeze(axis);
  if (components.size() == 1)
    return result;
  std::vector<Series> remaining;
  remaining.reserve(components.size() - 1);
  for (std::size_t index = 1; index < components.size(); ++index)
    remaining.push_back(components[index].unsqueeze(axis));
  return result.unite(remaining, axis);
}

} // namespace detail

[[nodiscard]] inline Series
partial(const Series &value, const PeriodicGrid &grid, std::int64_t axis) {
  constexpr const char *operation = "natsci::spatial::partial";
  detail::require_scalar_layout(value, grid, operation);
  detail::require_axis(grid, axis, operation);
  const std::int64_t series_axis = value.dim() - grid.dim() + axis;
  const double denominator =
      2.0 * grid.spacing_[static_cast<std::size_t>(axis)];
  return value.roll(-1, series_axis)
      .sub(value.roll(1, series_axis))
      .div(denominator);
}

// This forward/backward pair is adjoint-compatible on a periodic grid. Its
// composition is the three-point Laplacian used by pressure projection.
[[nodiscard]] inline Series
fwd_diff(const Series &value, const PeriodicGrid &grid, std::int64_t axis) {
  constexpr const char *operation = "natsci::spatial::fwd_diff";
  detail::require_scalar_layout(value, grid, operation);
  detail::require_axis(grid, axis, operation);
  const std::int64_t series_axis = value.dim() - grid.dim() + axis;
  return value.roll(-1, series_axis)
      .sub(value)
      .div(grid.spacing_[static_cast<std::size_t>(axis)]);
}

[[nodiscard]] inline Series
back_diff(const Series &value, const PeriodicGrid &grid, std::int64_t axis) {
  constexpr const char *operation = "natsci::spatial::back_diff";
  detail::require_scalar_layout(value, grid, operation);
  detail::require_axis(grid, axis, operation);
  const std::int64_t series_axis = value.dim() - grid.dim() + axis;
  return value.sub(value.roll(1, series_axis))
      .div(grid.spacing_[static_cast<std::size_t>(axis)]);
}

[[nodiscard]] inline Series second_partial(const Series &value,
                                           const PeriodicGrid &grid,
                                           std::int64_t axis) {
  constexpr const char *operation = "natsci::spatial::second_partial";
  detail::require_scalar_layout(value, grid, operation);
  detail::require_axis(grid, axis, operation);
  const std::int64_t series_axis = value.dim() - grid.dim() + axis;
  const double spacing = grid.spacing_[static_cast<std::size_t>(axis)];
  return value.roll(-1, series_axis)
      .add(value.roll(1, series_axis))
      .sub(value.mul(2.0))
      .div(spacing * spacing);
}

[[nodiscard]] inline Series gradient(const Series &value,
                                     const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::gradient";
  detail::require_scalar_layout(value, grid, operation);
  std::vector<Series> components;
  components.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis)
    components.push_back(partial(value, grid, axis));
  return detail::stack_components(components, value.dim() - grid.dim());
}

[[nodiscard]] inline Series fwd_grad(const Series &value,
                                     const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::fwd_grad";
  detail::require_scalar_layout(value, grid, operation);
  std::vector<Series> components;
  components.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis)
    components.push_back(fwd_diff(value, grid, axis));
  return detail::stack_components(components, value.dim() - grid.dim());
}

[[nodiscard]] inline Series divergence(const Series &value,
                                       const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::divergence";
  const std::int64_t component_axis =
      detail::require_vector_layout(value, grid, operation);
  Series result = partial(value.select(component_axis, 0), grid, 0);
  for (std::int64_t axis = 1; axis < grid.dim(); ++axis)
    result =
        result.add(partial(value.select(component_axis, axis), grid, axis));
  return result;
}

[[nodiscard]] inline Series back_div(const Series &value,
                                     const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::back_div";
  const std::int64_t component_axis =
      detail::require_vector_layout(value, grid, operation);
  Series result = back_diff(value.select(component_axis, 0), grid, 0);
  for (std::int64_t axis = 1; axis < grid.dim(); ++axis)
    result =
        result.add(back_diff(value.select(component_axis, axis), grid, axis));
  return result;
}

[[nodiscard]] inline Series laplacian(const Series &value,
                                      const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::laplacian";
  detail::require_scalar_layout(value, grid, operation);
  Series result = second_partial(value, grid, 0);
  for (std::int64_t axis = 1; axis < grid.dim(); ++axis)
    result = result.add(second_partial(value, grid, axis));
  return result;
}

// Second derivative with the two-cell centered stencil
// (u[i+2] - 2 u[i] + u[i-2]) / (4 h^2). It is exactly the composition
// divergence(gradient(u)) of the centered operators, whose symbol is
// -sin^2(k h) / h^2 per axis: a projection that pairs this operator with the
// centered gradient and divergence is second-order consistent on a
// cell-centered grid. It is positive-semidefinite after negation; its null
// space is the constant field plus the Nyquist checkerboard modes of even
// extents, which a centered divergence can never excite.
[[nodiscard]] inline Series wide_laplacian(const Series &value,
                                           const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::wide_laplacian";
  detail::require_scalar_layout(value, grid, operation);
  Series result = pluss::series::undefined();
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const std::int64_t series_axis = value.dim() - grid.dim() + axis;
    const double spacing = grid.spacing(axis).si_value();
    Series term = value.roll(-2, series_axis)
                      .add(value.roll(2, series_axis))
                      .sub(value.mul(2.0))
                      .div(4.0 * spacing * spacing);
    result = axis == 0 ? std::move(term) : result.add(term);
  }
  return result;
}

[[nodiscard]] inline Series curl(const Series &value,
                                 const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::curl";
  const std::int64_t component_axis =
      detail::require_vector_layout(value, grid, operation);
  if (grid.dim() == 2) {
    const Series x = value.select(component_axis, 0);
    const Series y = value.select(component_axis, 1);
    return partial(y, grid, 0).sub(partial(x, grid, 1));
  }
  if (grid.dim() != 3)
    throw std::invalid_argument(
        "natsci::spatial::curl: curl requires a two- or three-dimensional "
        "grid");

  const Series x = value.select(component_axis, 0);
  const Series y = value.select(component_axis, 1);
  const Series z = value.select(component_axis, 2);
  return detail::stack_components(
      {partial(z, grid, 1).sub(partial(y, grid, 2)),
       partial(x, grid, 2).sub(partial(z, grid, 0)),
       partial(y, grid, 0).sub(partial(x, grid, 1))},
      component_axis);
}

[[nodiscard]] inline Series advect(const Series &velocity, const Series &value,
                                   const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::advect";
  const std::int64_t velocity_axis =
      detail::require_vector_layout(velocity, grid, operation);
  detail::require_scalar_layout(value, grid, operation);
  if (velocity.scalar_type() != value.scalar_type())
    throw std::invalid_argument(
        "natsci::spatial::advect: velocity and field dtype mismatch");
  if (velocity.platform() != value.platform())
    throw std::invalid_argument(
        "natsci::spatial::advect: velocity and field platform mismatch");
  const std::int64_t velocity_batch_rank = velocity_axis;
  const std::int64_t value_batch_rank = value.dim() - grid.dim();
  if (velocity_batch_rank != value_batch_rank)
    throw std::invalid_argument(
        "natsci::spatial::advect: velocity and field batch rank mismatch");
  for (std::int64_t axis = 0; axis < value_batch_rank; ++axis) {
    if (velocity.size(axis) != value.size(axis))
      throw std::invalid_argument(
          "natsci::spatial::advect: velocity and field batch shape mismatch");
  }

  Series result =
      velocity.select(velocity_axis, 0).mul(partial(value, grid, 0));
  for (std::int64_t axis = 1; axis < grid.dim(); ++axis) {
    result = result.add(
        velocity.select(velocity_axis, axis).mul(partial(value, grid, axis)));
  }
  return result;
}

[[nodiscard]] inline Series vector_advection(const Series &velocity,
                                             const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::vector_advection";
  const std::int64_t component_axis =
      detail::require_vector_layout(velocity, grid, operation);
  std::vector<Series> components;
  components.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t component = 0; component < grid.dim(); ++component)
    components.push_back(
        advect(velocity, velocity.select(component_axis, component), grid));
  return detail::stack_components(components, component_axis);
}

// Outer product u_i u_j at every grid point. The two component axes are kept
// immediately before the trailing spatial axes so tensor_div can consume the
// result without reshaping or copying field data.
[[nodiscard]] inline Series vector_flux(const Series &velocity,
                                        const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::vector_flux";
  const std::int64_t component_axis =
      detail::require_vector_layout(velocity, grid, operation);
  return velocity.unsqueeze(component_axis + 1)
      .mul(velocity.unsqueeze(component_axis));
}

// Divergence over the second component axis: result_i = d_j tensor_ij.
[[nodiscard]] inline Series tensor_div(const Series &value,
                                       const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::tensor_div";
  const std::int64_t row_axis =
      detail::require_tensor_layout(value, grid, operation);
  std::vector<Series> components;
  components.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t row = 0; row < grid.dim(); ++row) {
    const Series row_field = value.select(row_axis, row);
    Series result = partial(row_field.select(row_axis, 0), grid, 0);
    for (std::int64_t axis = 1; axis < grid.dim(); ++axis)
      result =
          result.add(partial(row_field.select(row_axis, axis), grid, axis));
    components.push_back(std::move(result));
  }
  return detail::stack_components(components, row_axis);
}

[[nodiscard]] inline Series flux_div(const Series &velocity,
                                     const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::flux_div";
  detail::require_vector_layout(velocity, grid, operation);
  return tensor_div(vector_flux(velocity, grid), grid);
}

// The split form is energy-neutral under the periodic centered stencil:
// 1/2 [(u . grad)u + div(u outer u)]. It remains well defined when the
// discrete velocity is not exactly divergence free.
[[nodiscard]] inline Series skew_advect(const Series &velocity,
                                        const PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::spatial::skew_advect";
  detail::require_vector_layout(velocity, grid, operation);
  return vector_advection(velocity, grid)
      .add(flux_div(velocity, grid))
      .mul(0.5);
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Length>>
partial(const SeriesQuantity<D> &value, const PeriodicGrid &grid,
        std::int64_t axis) {
  return SeriesQuantity<DimensionQuotient<D, Length>>::from_si(
      partial(value.si_value(), grid, axis));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Length>>
fwd_diff(const SeriesQuantity<D> &value, const PeriodicGrid &grid,
         std::int64_t axis) {
  return SeriesQuantity<DimensionQuotient<D, Length>>::from_si(
      fwd_diff(value.si_value(), grid, axis));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Length>>
back_diff(const SeriesQuantity<D> &value, const PeriodicGrid &grid,
          std::int64_t axis) {
  return SeriesQuantity<DimensionQuotient<D, Length>>::from_si(
      back_diff(value.si_value(), grid, axis));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Area>>
second_partial(const SeriesQuantity<D> &value, const PeriodicGrid &grid,
               std::int64_t axis) {
  return SeriesQuantity<DimensionQuotient<D, Area>>::from_si(
      second_partial(value.si_value(), grid, axis));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Length>>
gradient(const SeriesQuantity<D> &value, const PeriodicGrid &grid) {
  return SeriesQuantity<DimensionQuotient<D, Length>>::from_si(
      gradient(value.si_value(), grid));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Length>>
fwd_grad(const SeriesQuantity<D> &value, const PeriodicGrid &grid) {
  return SeriesQuantity<DimensionQuotient<D, Length>>::from_si(
      fwd_grad(value.si_value(), grid));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Length>>
divergence(const SeriesQuantity<D> &value, const PeriodicGrid &grid) {
  return SeriesQuantity<DimensionQuotient<D, Length>>::from_si(
      divergence(value.si_value(), grid));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Length>>
back_div(const SeriesQuantity<D> &value, const PeriodicGrid &grid) {
  return SeriesQuantity<DimensionQuotient<D, Length>>::from_si(
      back_div(value.si_value(), grid));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Area>>
laplacian(const SeriesQuantity<D> &value, const PeriodicGrid &grid) {
  return SeriesQuantity<DimensionQuotient<D, Area>>::from_si(
      laplacian(value.si_value(), grid));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Area>>
wide_laplacian(const SeriesQuantity<D> &value, const PeriodicGrid &grid) {
  return SeriesQuantity<DimensionQuotient<D, Area>>::from_si(
      wide_laplacian(value.si_value(), grid));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Length>>
curl(const SeriesQuantity<D> &value, const PeriodicGrid &grid) {
  return SeriesQuantity<DimensionQuotient<D, Length>>::from_si(
      curl(value.si_value(), grid));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Time>>
advect(const SeriesQuantity<Speed> &velocity, const SeriesQuantity<D> &value,
       const PeriodicGrid &grid) {
  return SeriesQuantity<DimensionQuotient<D, Time>>::from_si(
      advect(velocity.si_value(), value.si_value(), grid));
}

[[nodiscard]] inline SeriesQuantity<Acceleration>
vector_advection(const SeriesQuantity<Speed> &velocity,
                 const PeriodicGrid &grid) {
  return SeriesQuantity<Acceleration>::from_si(
      vector_advection(velocity.si_value(), grid));
}

[[nodiscard]] inline SeriesQuantity<DimensionProduct<Speed, Speed>>
vector_flux(const SeriesQuantity<Speed> &velocity, const PeriodicGrid &grid) {
  return SeriesQuantity<DimensionProduct<Speed, Speed>>::from_si(
      vector_flux(velocity.si_value(), grid));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Length>>
tensor_div(const SeriesQuantity<D> &value, const PeriodicGrid &grid) {
  return SeriesQuantity<DimensionQuotient<D, Length>>::from_si(
      tensor_div(value.si_value(), grid));
}

[[nodiscard]] inline SeriesQuantity<Acceleration>
flux_div(const SeriesQuantity<Speed> &velocity, const PeriodicGrid &grid) {
  return SeriesQuantity<Acceleration>::from_si(
      flux_div(velocity.si_value(), grid));
}

[[nodiscard]] inline SeriesQuantity<Acceleration>
skew_advect(const SeriesQuantity<Speed> &velocity, const PeriodicGrid &grid) {
  return SeriesQuantity<Acceleration>::from_si(
      skew_advect(velocity.si_value(), grid));
}

} // namespace pluss::subsp::natsci::common::spatial
