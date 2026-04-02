#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <future>
#include <limits>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "CodeM/compat/Invoke.h"
#include "CodeM/core/Errors.h"
#include "CodeN/compat/Invoke.h"
#include "pluss/codeh/codeh.h"
#include "pluss/tils/aux/transmit.h"

namespace pluss::aux {

using transform_axis_options = transmit_options;

namespace detail {

inline std::int64_t wrap_axis(std::int64_t axis, std::int64_t dim, const char* where) {
  CODEM_CHECK(dim >= 0, std::string(where) + ": invalid rank");
  const std::int64_t d = axis < 0 ? axis + dim : axis;
  CODEM_CHECK(d >= 0 && d < dim, std::string(where) + ": axis out of range");
  return d;
}

template <class F, class SliceFn>
auto translate_slices(F&& fn,
                     std::size_t nslices,
                     SliceFn&& slice_fn,
                     transform_axis_options opts) {
  using slice_t = std::decay_t<decltype(slice_fn(std::size_t{0}))>;
  using out_t = invoke_result_elem_t<F&, const slice_t&>;

  std::vector<out_t> out;
  out.reserve(nslices);
  if (nslices == 0) return out;

  if (opts.exec == transmit_exec::par &&
      opts.grain_size > 0 &&
      std::default_initializable<out_t>) {
    out.resize(nslices);
    const std::size_t hw = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    const std::size_t workers = std::max<std::size_t>(1, opts.workers == 0 ? hw : opts.workers);
    const std::size_t chunks = std::max<std::size_t>(
        1, std::min<std::size_t>(workers, (nslices + opts.grain_size - 1) / opts.grain_size));
    const std::size_t block = (nslices + chunks - 1) / chunks;
    auto fn_copy = std::forward<F>(fn);

    std::vector<std::future<void>> futures;
    futures.reserve(chunks);
    for (std::size_t c = 0; c < chunks; ++c) {
      const std::size_t start = c * block;
      const std::size_t end = std::min<std::size_t>(nslices, start + block);
      futures.emplace_back(std::async(std::launch::async, [&, start, end]() {
        for (std::size_t i = start; i < end; ++i) {
          const auto slice = slice_fn(i);
          out[i] = invoke_with_optional_index(fn_copy, slice, i);
        }
      }));
    }
    for (auto& f : futures) f.get();
    return out;
  }

  for (std::size_t i = 0; i < nslices; ++i) {
    const auto slice = slice_fn(i);
    out.emplace_back(invoke_with_optional_index(fn, slice, i));
  }
  return out;
}

struct axis_layout {
  std::vector<std::int64_t> fixed_dims;
  std::vector<std::int64_t> fixed_shape;
  std::size_t nslices{1};
};

template <class SizeFn>
axis_layout make_axis_layout(std::int64_t dim,
                             std::int64_t axis,
                             SizeFn&& size_fn,
                             const char* where) {
  axis_layout layout;
  if (dim <= 1) return layout;
  layout.fixed_dims.reserve(static_cast<std::size_t>(dim - 1));
  layout.fixed_shape.reserve(static_cast<std::size_t>(dim - 1));

  for (std::int64_t k = 0; k < dim; ++k) {
    if (k == axis) continue;
    const auto sz = size_fn(k);
    CODEM_CHECK(sz >= 0, std::string(where) + ": invalid shape");
    layout.fixed_dims.push_back(k);
    layout.fixed_shape.push_back(sz);
    if (sz == 0) {
      layout.nslices = 0;
      continue;
    }
    CODEM_CHECK(
        layout.nslices <= std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(sz),
        std::string(where) + ": slice space too large");
    layout.nslices *= static_cast<std::size_t>(sz);
  }
  return layout;
}

inline std::vector<std::int64_t> unravel_index(std::size_t linear,
                                               const std::vector<std::int64_t>& shape) {
  std::vector<std::int64_t> coord(shape.size(), 0);
  for (std::size_t i = shape.size(); i-- > 0;) {
    const auto size_i = static_cast<std::size_t>(shape[i]);
    if (size_i == 0) continue;
    coord[i] = static_cast<std::int64_t>(linear % size_i);
    linear /= size_i;
  }
  return coord;
}

} // namespace detail

// NumPy-style apply_along_axis for Series (up to CodeM max rank).
// Applies fn to each 1D slice oriented along `axis`.
template <class F>
auto transform_axis(F&& fn,
                    const pluss::Series& x,
                    std::int64_t axis,
                    transform_axis_options opts = {}) {
  CODEM_CHECK(x.defined(), "transform_axis(Series): undefined input");
  CODEM_CHECK(x.dim() > 0, "transform_axis(Series): axis dispatch requires dim > 0");
  const auto d = detail::wrap_axis(axis, x.dim(), "transform_axis(Series)");
  const auto sizes = x.sizes();
  const auto layout = detail::make_axis_layout(
      x.dim(),
      d,
      [&](std::int64_t k) {
        return sizes[static_cast<std::size_t>(k)];
      },
      "transform_axis(Series)");

  auto make_slice = [&](std::size_t linear_index) {
    const auto fixed_index = detail::unravel_index(linear_index, layout.fixed_shape);
    auto slice = x;
    for (std::size_t p = layout.fixed_dims.size(); p-- > 0;) {
      slice = ::pluss::codem::_compat::invoke(
          "select",
          slice,
          layout.fixed_dims[p],
          fixed_index[p],
          static_cast<std::int64_t>(1));
    }
    return slice;
  };

  return detail::translate_slices(std::forward<F>(fn), layout.nslices, make_slice, opts);
}

// NumPy-style apply_along_axis for Note (0D..3D).
template <class F>
auto transform_axis(F&& fn,
                    const pluss::Note& x,
                    std::int64_t axis,
                    transform_axis_options opts = {}) {
  CODEM_CHECK(x.defined(), "transform_axis(Note): undefined input");
  CODEM_CHECK(x.dim() > 0, "transform_axis(Note): axis dispatch requires dim > 0");
  const auto d = detail::wrap_axis(axis, x.dim(), "transform_axis(Note)");
  const auto layout = detail::make_axis_layout(
      x.dim(),
      d,
      [&](std::int64_t k) { return x.size(k); },
      "transform_axis(Note)");

  auto make_slice = [&](std::size_t linear_index) {
    const auto fixed_index = detail::unravel_index(linear_index, layout.fixed_shape);
    auto slice = x;
    for (std::size_t p = layout.fixed_dims.size(); p-- > 0;) {
      slice = ::pluss::coden::_compat::invoke<pluss::Note>(
          "select",
          slice,
          layout.fixed_dims[p],
          fixed_index[p]);
    }
    return slice;
  };

  return detail::translate_slices(std::forward<F>(fn), layout.nslices, make_slice, opts);
}

// NumPy-style apply_along_axis for Timeline (0D..3D).
// Slices are returned as 1D Timeline views reconstructed from selected tick Series.
template <class F>
auto transform_axis(F&& fn,
                    const pluss::codeh::Timeline& x,
                    std::int64_t axis,
                    transform_axis_options opts = {}) {
  CODEM_CHECK(x.defined(), "transform_axis(Timeline): undefined input");
  CODEM_CHECK(x.dim() > 0, "transform_axis(Timeline): axis dispatch requires dim > 0");
  const auto d = detail::wrap_axis(axis, x.dim(), "transform_axis(Timeline)");
  const auto sizes = x.sizes();
  const auto layout = detail::make_axis_layout(
      x.dim(),
      d,
      [&](std::int64_t k) {
        return sizes[static_cast<std::size_t>(k)];
      },
      "transform_axis(Timeline)");

  auto make_slice = [&](std::size_t linear_index) {
    const auto fixed_index = detail::unravel_index(linear_index, layout.fixed_shape);
    auto ticks_slice = x.unsafe_ticks();
    for (std::size_t p = layout.fixed_dims.size(); p-- > 0;) {
      ticks_slice = ::pluss::codem::_compat::invoke(
          "select",
          ticks_slice,
          layout.fixed_dims[p],
          fixed_index[p],
          static_cast<std::int64_t>(1));
    }
    return ::pluss::codeh::Timeline::fromSeries(ticks_slice);
  };

  return detail::translate_slices(std::forward<F>(fn), layout.nslices, make_slice, opts);
}

// TimeIndex is 1D by design. axis must be 0 or -1.
template <class F>
auto transform_axis(F&& fn,
                    const pluss::codeh::TimeIndex& x,
                    std::int64_t axis = 0,
                    transform_axis_options opts = {}) {
  (void)opts;
  const auto d = detail::wrap_axis(axis, 1, "transform_axis(TimeIndex)");
  (void)d;
  const auto t = x.timeline();
  return detail::invoke_with_optional_index(std::forward<F>(fn), t, 0);
}

} // namespace pluss::aux

namespace pluss {
using aux::transform_axis;
using aux::transform_axis_options;
} // namespace pluss
