#pragma once

#include "pluss/codem/series_factory.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace pluss::subsp::math::interp {

enum class EdgeMode { Clamp, Linear, Error };

// Linear interpolation on a shared one-dimensional coordinate. Values use
// shape [..., n], queries are scalar or [m], and output is [...] or [..., m].
// Leading value axes are batches; interval selection and gathering stay on the
// Series backend.
[[nodiscard]] inline Series linear(const Series &coordinate,
                                   const Series &values, const Series &query,
                                   EdgeMode edge = EdgeMode::Clamp) {
  if (!coordinate.defined() || !values.defined() || !query.defined())
    throw std::invalid_argument(
        "math::interp::linear: expected defined Series values");
  if (coordinate.dim() != 1 || coordinate.size(0) < 2)
    throw std::invalid_argument("math::interp::linear: coordinate must be "
                                "one-dimensional with at least two points");
  if (query.dim() > 1)
    throw std::invalid_argument(
        "math::interp::linear: query must be scalar or one-dimensional");
  if (values.dim() < 1 || values.size(-1) != coordinate.size(0))
    throw std::invalid_argument(
        "math::interp::linear: values must have shape [..., coordinate.size]");
  if (!coordinate.dtype().is_floating() || !query.dtype().is_floating())
    throw std::invalid_argument("math::interp::linear: coordinate and query "
                                "must be real floating Series");
  if (!values.dtype().is_floating() && !values.dtype().is_complex())
    throw std::invalid_argument(
        "math::interp::linear: values must be floating or complex");
  if (coordinate.scalar_type() != query.scalar_type())
    throw std::invalid_argument(
        "math::interp::linear: coordinate and query dtype mismatch");
  if (coordinate.platform() != query.platform() ||
      coordinate.platform() != values.platform())
    throw std::invalid_argument("math::interp::linear: coordinate, values, and "
                                "query must share a platform");
  if (!coordinate.isfinite().all().item<bool>() ||
      !query.isfinite().all().item<bool>() ||
      !(values.dtype().is_complex() ? values.abs() : values).isfinite().all().item<bool>())
    throw std::invalid_argument(
        "math::interp::linear: coordinate, values, and query must be finite");
  if (!coordinate.diff(1, 0).gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "math::interp::linear: coordinate must be strictly increasing");

  const bool scalar_query = query.dim() == 0;
  Series points = query.reshape({query.numel()});
  const Series first = coordinate.select(0, 0);
  const Series last = coordinate.select(0, coordinate.size(0) - 1);
  if (edge == EdgeMode::Error &&
      !points.ge(first).logical_and(points.le(last)).all().item<bool>())
    throw std::out_of_range(
        "math::interp::linear: query lies outside the coordinate domain");
  if (edge == EdgeMode::Clamp)
    points = points.maximum(first).minimum(last);

  const auto count = coordinate.size(0);
  Series index = points.unsqueeze(-1)
                     .ge(coordinate.reshape({1, count}))
                     .sum(-1, false)
                     .sub(1)
                     .clamp(0.0, static_cast<double>(count - 2))
                     .to(ScalarType::Int64);
  const Series next_index = index.add(1);
  const Series x0 = coordinate.take(index);
  const Series x1 = coordinate.take(next_index);
  const Series weight = points.sub(x0).div(x1.sub(x0));

  std::vector<std::int64_t> gather_shape(static_cast<std::size_t>(values.dim()),
                                         1);
  gather_shape.back() = points.numel();
  Series gather_index = index.reshape(gather_shape);
  std::vector<std::int64_t> output_shape(values.sizes().begin(),
                                         values.sizes().end());
  output_shape.back() = points.numel();
  gather_index = gather_index.expand(output_shape);
  const Series y0 = values.gather(-1, gather_index);
  const Series y1 = values.gather(-1, gather_index.add(1));

  std::vector<std::int64_t> weight_shape(static_cast<std::size_t>(values.dim()),
                                         1);
  weight_shape.back() = points.numel();
  const Series aligned_weight =
      weight.reshape(weight_shape).to(values.scalar_type());
  Series output = y0.add(y1.sub(y0).mul(aligned_weight));
  return scalar_query ? output.squeeze(-1) : output;
}

} // namespace pluss::subsp::math::interp
