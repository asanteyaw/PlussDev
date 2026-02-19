#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <future>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "CodeM/compat/Invoke.h"
#include "CodeM/core/Errors.h"
#include "CodeN/compat/Invoke.h"
#include "pluss/codeh/codeh.h"
#include "aux/transmit.h"

namespace pluss::aux {

using translate_options = transmit_options;

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
                     translate_options opts) {
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

} // namespace detail

// NumPy-style apply_along_axis for Series (up to CodeM max rank).
// Returns one result per slice in order along `axis`.
template <class F>
auto translate(F&& fn,
              const pluss::Series& x,
              std::int64_t axis,
              translate_options opts = {}) {
  CODEM_CHECK(x.defined(), "translate(Series): undefined input");
  CODEM_CHECK(x.dim() > 0, "translate(Series): axis dispatch requires dim > 0");
  const auto d = detail::wrap_axis(axis, x.dim(), "translate(Series)");
  const auto nslices = static_cast<std::size_t>(x.sizes()[static_cast<std::size_t>(d)]);

  auto make_slice = [&](std::size_t i) {
    return ::pluss::codem::_compat::invoke(
        "select",
        x,
        d,
        static_cast<std::int64_t>(i),
        static_cast<std::int64_t>(1));
  };

  return detail::translate_slices(std::forward<F>(fn), nslices, make_slice, opts);
}

// NumPy-style apply_along_axis for Note (0D..3D).
template <class F>
auto translate(F&& fn,
              const pluss::Note& x,
              std::int64_t axis,
              translate_options opts = {}) {
  CODEM_CHECK(x.defined(), "translate(Note): undefined input");
  CODEM_CHECK(x.dim() > 0, "translate(Note): axis dispatch requires dim > 0");
  const auto d = detail::wrap_axis(axis, x.dim(), "translate(Note)");
  const auto nslices = static_cast<std::size_t>(x.size(d));

  auto make_slice = [&](std::size_t i) {
    return ::pluss::coden::_compat::invoke<pluss::Note>(
        "select",
        x,
        d,
        static_cast<std::int64_t>(i));
  };

  return detail::translate_slices(std::forward<F>(fn), nslices, make_slice, opts);
}

// NumPy-style apply_along_axis for Timeline (0D..3D).
// Slices are returned as Timeline views reconstructed from selected tick Series.
template <class F>
auto translate(F&& fn,
              const pluss::codeh::Timeline& x,
              std::int64_t axis,
              translate_options opts = {}) {
  CODEM_CHECK(x.defined(), "translate(Timeline): undefined input");
  CODEM_CHECK(x.dim() > 0, "translate(Timeline): axis dispatch requires dim > 0");
  const auto d = detail::wrap_axis(axis, x.dim(), "translate(Timeline)");
  const auto sizes = x.sizes();
  const auto nslices = static_cast<std::size_t>(sizes[static_cast<std::size_t>(d)]);

  auto make_slice = [&](std::size_t i) {
    const auto selected_ticks = ::pluss::codem::_compat::invoke(
        "select",
        x.unsafe_ticks(),
        d,
        static_cast<std::int64_t>(i),
        static_cast<std::int64_t>(1));
    return ::pluss::codeh::Timeline::fromSeries(selected_ticks);
  };

  return detail::translate_slices(std::forward<F>(fn), nslices, make_slice, opts);
}

// TimeIndex is 1D by design. axis must be 0 or -1.
template <class F>
auto translate(F&& fn,
              const pluss::codeh::TimeIndex& x,
              std::int64_t axis = 0,
              translate_options opts = {}) {
  (void)opts;
  const auto d = detail::wrap_axis(axis, 1, "translate(TimeIndex)");
  (void)d;
  const auto t = x.timeline();
  return detail::invoke_with_optional_index(std::forward<F>(fn), t, 0);
}

} // namespace pluss::aux

namespace pluss {
using aux::translate;
using aux::translate_options;
} // namespace pluss
