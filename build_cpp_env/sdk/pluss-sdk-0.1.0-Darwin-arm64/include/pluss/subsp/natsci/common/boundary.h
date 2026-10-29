#pragma once

#include "pluss/subsp/natsci/common/field.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace pluss::subsp::natsci::common::spatial {

template <DimensionType D> class Bounds;
template <DimensionType D> struct GhostPair;

class BoxGrid final {
public:
  [[nodiscard]] std::int64_t dim() const noexcept {
    return static_cast<std::int64_t>(shape_.size());
  }

  [[nodiscard]] const std::vector<std::int64_t> &shape() const noexcept {
    return shape_;
  }

  [[nodiscard]] ScalarQuantity<Length> spacing(std::int64_t axis) const {
    require_axis(axis, "BoxGrid::spacing");
    return ScalarQuantity<Length>::from_si(
        spacing_[static_cast<std::size_t>(axis)]);
  }

private:
  BoxGrid(std::vector<std::int64_t> shape, std::vector<double> spacing)
      : shape_(std::move(shape)), spacing_(std::move(spacing)) {}

  void require_axis(std::int64_t axis, const char *operation) const {
    if (axis < 0 || axis >= dim())
      throw std::out_of_range(std::string(operation) +
                              ": spatial axis is out of range");
  }

  std::vector<std::int64_t> shape_;
  std::vector<double> spacing_;

  friend BoxGrid make_box_grid(std::vector<std::int64_t>,
                               std::vector<ScalarQuantity<Length>>);
  template <DimensionType D> friend struct GhostPair;
  template <DimensionType D>
  friend GhostPair<D> ghosts(const SeriesQuantity<D> &, const BoxGrid &,
                             const Bounds<D> &, std::int64_t);
};

[[nodiscard]] inline BoxGrid
make_box_grid(std::vector<std::int64_t> shape,
              std::vector<ScalarQuantity<Length>> spacing) {
  if (shape.empty() || shape.size() > 3)
    throw std::invalid_argument(
        "natsci::spatial::make_box_grid: expected one to three spatial axes");
  if (shape.size() != spacing.size())
    throw std::invalid_argument(
        "natsci::spatial::make_box_grid: shape and spacing rank mismatch");

  std::vector<double> spacing_si;
  spacing_si.reserve(spacing.size());
  for (std::size_t axis = 0; axis < shape.size(); ++axis) {
    if (shape[axis] < 2)
      throw std::invalid_argument(
          "natsci::spatial::make_box_grid: every extent must be at least two");
    const double value = spacing[axis].si_value();
    if (!(value > 0.0) || !std::isfinite(value))
      throw std::invalid_argument("natsci::spatial::make_box_grid: spacing "
                                  "must be finite and positive");
    spacing_si.push_back(value);
  }
  return BoxGrid(std::move(shape), std::move(spacing_si));
}

enum class Side { Low, High };

struct Face final {
  std::int64_t axis{};
  Side side{Side::Low};
};

enum class BoundKind { Value, Flux };

namespace detail {

using BoundData = std::variant<double, Series>;

template <DimensionType D, QuantityRep Rep>
[[nodiscard]] inline BoundData pack_bound(const Quantity<D, Rep> &value) {
  if constexpr (std::same_as<Rep, double>)
    return value.si_value();
  else
    return value.si_value();
}

inline std::size_t face_slot(Face face) {
  return static_cast<std::size_t>(2 * face.axis +
                                  (face.side == Side::High ? 1 : 0));
}

inline void require_face(Face face, const BoxGrid &grid,
                         const char *operation) {
  if (face.axis < 0 || face.axis >= grid.dim())
    throw std::out_of_range(std::string(operation) +
                            ": face axis is out of range");
}

template <class Rule, class FaceOf>
[[nodiscard]] inline std::vector<Rule>
order_faces(const BoxGrid &grid, const std::vector<Rule> &rules, FaceOf face_of,
            const char *operation) {
  const auto expected = static_cast<std::size_t>(2 * grid.dim());
  if (rules.size() != expected)
    throw std::invalid_argument(std::string(operation) +
                                ": expected exactly one rule per face");

  std::vector<bool> seen(expected, false);
  for (const auto &rule : rules) {
    const Face face = face_of(rule);
    require_face(face, grid, operation);
    const auto slot = face_slot(face);
    if (seen[slot])
      throw std::invalid_argument(std::string(operation) +
                                  ": duplicate face rule");
    seen[slot] = true;
  }

  std::vector<Rule> ordered;
  ordered.reserve(expected);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    for (const Side side : {Side::Low, Side::High}) {
      const auto slot = face_slot({axis, side});
      if (!seen[slot])
        throw std::invalid_argument(std::string(operation) +
                                    ": missing face rule");
      for (const auto &rule : rules) {
        const Face face = face_of(rule);
        if (face.axis == axis && face.side == side) {
          ordered.push_back(rule);
          break;
        }
      }
    }
  }
  return ordered;
}

inline void require_scalar_layout(const Series &value, const BoxGrid &grid,
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
                                          const BoxGrid &grid,
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

inline void require_axis(const BoxGrid &grid, std::int64_t axis,
                         const char *operation) {
  if (axis < 0 || axis >= grid.dim())
    throw std::out_of_range(std::string(operation) +
                            ": spatial axis is out of range");
}

inline bool same_shape(const Series &lhs, const Series &rhs) noexcept {
  if (lhs.dim() != rhs.dim())
    return false;
  for (std::int64_t axis = 0; axis < lhs.dim(); ++axis)
    if (lhs.size(axis) != rhs.size(axis))
      return false;
  return true;
}

[[nodiscard]] inline Series
face_data(const BoundData &data, const Series &plane, const char *operation) {
  if (const auto *scalar = std::get_if<double>(&data))
    return pluss::codem::make_scalar_like(plane, *scalar);

  const Series &profile = std::get<Series>(data);
  if (profile.scalar_type() != plane.scalar_type())
    throw std::invalid_argument(std::string(operation) +
                                ": boundary profile dtype mismatch");
  if (profile.platform() != plane.platform())
    throw std::invalid_argument(std::string(operation) +
                                ": boundary profile platform mismatch");
  Series expanded;
  try {
    expanded =
        plane.mul(pluss::codem::make_scalar_like(plane, 0.0)).add(profile);
  } catch (const std::exception &) {
    throw std::invalid_argument(std::string(operation) +
                                ": boundary profile is not broadcastable to "
                                "the face plane");
  }
  if (!same_shape(expanded, plane))
    throw std::invalid_argument(std::string(operation) +
                                ": boundary profile changed the face shape");
  return expanded;
}

} // namespace detail

template <DimensionType D> class Bound final {
public:
  [[nodiscard]] Face face() const noexcept { return face_; }
  [[nodiscard]] BoundKind kind() const noexcept { return kind_; }

  template <QuantityRep Rep>
  [[nodiscard]] static Bound value(Face face, const Quantity<D, Rep> &value) {
    return Bound(face, BoundKind::Value, detail::pack_bound(value));
  }

  template <QuantityRep Rep>
  [[nodiscard]] static Bound
  flux(Face face, const Quantity<DimensionQuotient<D, Length>, Rep> &value) {
    return Bound(face, BoundKind::Flux, detail::pack_bound(value));
  }

private:
  Bound(Face face, BoundKind kind, detail::BoundData data)
      : face_(face), kind_(kind), data_(std::move(data)) {}

  Face face_;
  BoundKind kind_;
  detail::BoundData data_;

  template <DimensionType OtherD> friend class Bounds;
  template <DimensionType OtherD>
  friend GhostPair<OtherD> ghosts(const SeriesQuantity<OtherD> &,
                                  const BoxGrid &, const Bounds<OtherD> &,
                                  std::int64_t);
};

template <DimensionType D, QuantityRep Rep>
[[nodiscard]] inline Bound<D> value_bc(Face face,
                                       const Quantity<D, Rep> &value) {
  return Bound<D>::value(face, value);
}

template <DimensionType FluxD, QuantityRep Rep>
[[nodiscard]] inline Bound<DimensionProduct<FluxD, Length>>
flux_bc(Face face, const Quantity<FluxD, Rep> &value) {
  using ValueD = DimensionProduct<FluxD, Length>;
  return Bound<ValueD>::flux(face, value);
}

template <DimensionType D> class Bounds final {
public:
  [[nodiscard]] const Bound<D> &at(std::int64_t axis, Side side) const {
    if (axis < 0 || axis >= dim_)
      throw std::out_of_range(
          "natsci::spatial::Bounds::at: face axis is out of range");
    return rules_[detail::face_slot(Face{axis, side})];
  }

  [[nodiscard]] std::int64_t dim() const noexcept { return dim_; }

  [[nodiscard]] bool all_flux() const noexcept {
    for (const auto &rule : rules_)
      if (rule.kind() != BoundKind::Flux)
        return false;
    return true;
  }

  [[nodiscard]] Bounds zeroed() const {
    std::vector<Bound<D>> rules;
    rules.reserve(rules_.size());
    for (const auto &rule : rules_) {
      if (rule.kind() == BoundKind::Value)
        rules.push_back(
            Bound<D>::value(rule.face(), ScalarQuantity<D>::from_si(0.0)));
      else
        rules.push_back(Bound<D>::flux(
            rule.face(),
            ScalarQuantity<DimensionQuotient<D, Length>>::from_si(0.0)));
    }
    return Bounds(dim_, std::move(rules));
  }

private:
  Bounds(std::int64_t dim, std::vector<Bound<D>> rules)
      : dim_(dim), rules_(std::move(rules)) {}

  std::int64_t dim_{};
  std::vector<Bound<D>> rules_;

  template <DimensionType OtherD>
  friend Bounds<OtherD> make_bounds(const BoxGrid &,
                                    std::vector<Bound<OtherD>>);
};

template <DimensionType D>
[[nodiscard]] inline Bounds<D> make_bounds(const BoxGrid &grid,
                                           std::vector<Bound<D>> rules) {
  constexpr const char *operation = "natsci::spatial::make_bounds";
  auto ordered = detail::order_faces(
      grid, rules, [](const Bound<D> &rule) { return rule.face(); }, operation);
  return Bounds<D>(grid.dim(), std::move(ordered));
}

template <DimensionType D>
[[nodiscard]] inline Bounds<D>
make_bounds(const BoxGrid &grid, std::initializer_list<Bound<D>> rules) {
  return make_bounds(grid, std::vector<Bound<D>>(rules));
}

template <DimensionType D> struct GhostPair final {
  SeriesQuantity<D> low;
  SeriesQuantity<D> high;
};

template <DimensionType D>
[[nodiscard]] inline GhostPair<D>
ghosts(const SeriesQuantity<D> &value, const BoxGrid &grid,
       const Bounds<D> &bounds, std::int64_t axis) {
  constexpr const char *operation = "natsci::spatial::ghosts";
  const Series &field = value.si_value();
  detail::require_scalar_layout(field, grid, operation);
  detail::require_axis(grid, axis, operation);
  if (bounds.dim() != grid.dim())
    throw std::invalid_argument(
        "natsci::spatial::ghosts: boundary and grid rank mismatch");

  const std::int64_t series_axis = field.dim() - grid.dim() + axis;
  const double spacing = grid.spacing_[static_cast<std::size_t>(axis)];
  const auto reconstruct = [&](Side side) {
    const Bound<D> &rule = bounds.at(axis, side);
    const std::int64_t index =
        side == Side::Low ? 0 : field.size(series_axis) - 1;
    const Series interior = field.select(series_axis, index);
    const Series data = detail::face_data(rule.data_, interior, operation);
    Series result =
        rule.kind() == BoundKind::Value
            ? data.mul(pluss::codem::make_scalar_like(data, 2.0)).sub(interior)
            : interior.add(
                  data.mul(pluss::codem::make_scalar_like(data, spacing)));
    return result.reshape(std::vector<std::int64_t>(interior.sizes().begin(),
                                                    interior.sizes().end()));
  };

  return {SeriesQuantity<D>::from_si(reconstruct(Side::Low)),
          SeriesQuantity<D>::from_si(reconstruct(Side::High))};
}

namespace detail {

template <DimensionType D>
[[nodiscard]] inline std::pair<Series, Series>
neighbors(const SeriesQuantity<D> &value, const BoxGrid &grid,
          const Bounds<D> &bounds, std::int64_t axis, const char *operation) {
  const Series &field = value.si_value();
  require_scalar_layout(field, grid, operation);
  require_axis(grid, axis, operation);
  const std::int64_t series_axis = field.dim() - grid.dim() + axis;
  const std::int64_t extent = field.size(series_axis);
  const auto ghost = ghosts(value, grid, bounds, axis);
  Series lower =
      ghost.low.si_value()
          .unsqueeze(series_axis)
          .unite({field.slice(series_axis, 0, extent - 1)}, series_axis);
  Series upper =
      field.slice(series_axis, 1, extent)
          .unite({ghost.high.si_value().unsqueeze(series_axis)}, series_axis);
  return {std::move(lower), std::move(upper)};
}

} // namespace detail

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Length>>
partial(const SeriesQuantity<D> &value, const BoxGrid &grid,
        const Bounds<D> &bounds, std::int64_t axis) {
  auto [lower, upper] =
      detail::neighbors(value, grid, bounds, axis, "natsci::spatial::partial");
  return SeriesQuantity<DimensionQuotient<D, Length>>::from_si(
      upper.sub(lower).div(pluss::codem::make_scalar_like(
          upper, 2.0 * grid.spacing(axis).si_value())));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Area>>
second_partial(const SeriesQuantity<D> &value, const BoxGrid &grid,
               const Bounds<D> &bounds, std::int64_t axis) {
  auto [lower, upper] = detail::neighbors(value, grid, bounds, axis,
                                          "natsci::spatial::second_partial");
  const double spacing = grid.spacing(axis).si_value();
  const Series two = pluss::codem::make_scalar_like(value.si_value(), 2.0);
  const Series area =
      pluss::codem::make_scalar_like(value.si_value(), spacing * spacing);
  return SeriesQuantity<DimensionQuotient<D, Area>>::from_si(
      lower.add(upper).sub(value.si_value().mul(two)).div(area));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Length>>
gradient(const SeriesQuantity<D> &value, const BoxGrid &grid,
         const Bounds<D> &bounds) {
  detail::require_scalar_layout(value.si_value(), grid,
                                "natsci::spatial::gradient");
  std::vector<Series> components;
  components.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis)
    components.push_back(partial(value, grid, bounds, axis).si_value());
  return SeriesQuantity<DimensionQuotient<D, Length>>::from_si(
      detail::stack_components(components,
                               value.si_value().dim() - grid.dim()));
}

template <DimensionType D>
[[nodiscard]] inline SeriesQuantity<DimensionQuotient<D, Area>>
laplacian(const SeriesQuantity<D> &value, const BoxGrid &grid,
          const Bounds<D> &bounds) {
  detail::require_scalar_layout(value.si_value(), grid,
                                "natsci::spatial::laplacian");
  Series result = second_partial(value, grid, bounds, 0).si_value();
  for (std::int64_t axis = 1; axis < grid.dim(); ++axis)
    result = result.add(second_partial(value, grid, bounds, axis).si_value());
  return SeriesQuantity<DimensionQuotient<D, Area>>::from_si(std::move(result));
}

} // namespace pluss::subsp::natsci::common::spatial
