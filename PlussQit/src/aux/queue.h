#pragma once

#include <concepts>
#include <cstddef>
#include <future>
#include <functional>
#include <stdexcept>
#include <iterator>
#include <ranges>
#include <span>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::aux {

enum class queue_exec {
  seq,
  par
};

struct queue_options {
  queue_exec exec{queue_exec::seq};
  std::size_t grain_size{4096};
  std::size_t workers{0};
};

namespace detail {

template <class F, class Elem>
constexpr decltype(auto) invoke_with_optional_index(F&& fn, Elem&& elem, std::size_t index) {
  if constexpr (std::is_invocable_v<F, Elem, std::size_t>) {
    return std::invoke(std::forward<F>(fn), std::forward<Elem>(elem), index);
  } else {
    static_assert(std::is_invocable_v<F, Elem>,
                  "queue: callable must accept either (elem) or (elem, index)");
    return std::invoke(std::forward<F>(fn), std::forward<Elem>(elem));
  }
}

template <class F, class Elem>
using invoke_result_elem_t = std::decay_t<decltype(
    invoke_with_optional_index(std::declval<F>(), std::declval<Elem>(), std::declval<std::size_t>()))>;

template <class F, class A, class B>
constexpr decltype(auto) invoke_binary(F&& fn, A&& a, B&& b, std::size_t index) {
  if constexpr (std::is_invocable_v<F, A, B, std::size_t>) {
    return std::invoke(std::forward<F>(fn), std::forward<A>(a), std::forward<B>(b), index);
  } else {
    static_assert(std::is_invocable_v<F, A, B>,
                  "queue(binary): callable must accept (a, b) or (a, b, index)");
    return std::invoke(std::forward<F>(fn), std::forward<A>(a), std::forward<B>(b));
  }
}

} // namespace detail

template <class F, class T>
requires (!std::ranges::input_range<std::remove_reference_t<T>>)
constexpr decltype(auto) queue(F&& fn, T&& value) {
  return detail::invoke_with_optional_index(std::forward<F>(fn), std::forward<T>(value), 0);
}

template <class F, std::ranges::input_range R>
auto queue(F&& fn, R&& input, queue_options opts = {}) {
  using ref_t = std::ranges::range_reference_t<R>;
  using out_t = detail::invoke_result_elem_t<F&, ref_t>;
  std::vector<out_t> out;

  if constexpr (std::ranges::sized_range<R>) {
    out.reserve(static_cast<std::size_t>(std::ranges::size(input)));
  }

  if (opts.exec == queue_exec::par &&
      opts.grain_size > 0 &&
      std::ranges::random_access_range<R> &&
      std::ranges::sized_range<R> &&
      std::default_initializable<out_t>) {
    const std::size_t n = static_cast<std::size_t>(std::ranges::size(input));
    out.resize(n);
    if (n == 0) return out;

    const std::size_t hw = std::max<std::size_t>(1, std::thread::hardware_concurrency());
    const std::size_t workers = std::max<std::size_t>(1, opts.workers == 0 ? hw : opts.workers);
    const std::size_t chunks = std::max<std::size_t>(
        1, std::min<std::size_t>(workers, (n + opts.grain_size - 1) / opts.grain_size));
    const std::size_t block = (n + chunks - 1) / chunks;
    auto begin = std::ranges::begin(input);

    auto fn_copy = std::forward<F>(fn);
    std::vector<std::future<void>> futures;
    futures.reserve(chunks);
    for (std::size_t c = 0; c < chunks; ++c) {
      const std::size_t start = c * block;
      const std::size_t end = std::min<std::size_t>(n, start + block);
      futures.emplace_back(std::async(std::launch::async, [&, start, end]() {
        for (std::size_t i = start; i < end; ++i) {
          out[i] = detail::invoke_with_optional_index(fn_copy, *(begin + static_cast<std::ptrdiff_t>(i)), i);
        }
      }));
    }
    for (auto& f : futures) f.get();
    return out;
  }

  std::size_t i = 0;
  for (auto&& elem : input) {
    out.emplace_back(detail::invoke_with_optional_index(fn, elem, i++));
  }
  return out;
}

template <class F, class It, class Sent>
requires std::input_iterator<It> && std::sentinel_for<Sent, It>
auto queue(F&& fn, It first, Sent last, queue_options opts = {}) {
  (void)opts;
  using ref_t = std::iter_reference_t<It>;
  using out_t = detail::invoke_result_elem_t<F&, ref_t>;
  std::vector<out_t> out;
  std::size_t i = 0;
  for (; first != last; ++first) {
    out.emplace_back(detail::invoke_with_optional_index(fn, *first, i++));
  }
  return out;
}

template <class F, std::ranges::input_range R, class OutIt>
OutIt queue_into(F&& fn, R&& input, OutIt out) {
  std::size_t i = 0;
  for (auto&& elem : input) {
    *out++ = detail::invoke_with_optional_index(fn, elem, i++);
  }
  return out;
}

template <class F, std::ranges::input_range R1, std::ranges::input_range R2>
auto queue(F&& fn, R1&& a, R2&& b) {
  using a_ref = std::ranges::range_reference_t<R1>;
  using b_ref = std::ranges::range_reference_t<R2>;
  using out_t = std::decay_t<decltype(
      detail::invoke_binary(std::declval<F&>(), std::declval<a_ref>(), std::declval<b_ref>(), std::declval<std::size_t>()))>;

  auto it_a = std::ranges::begin(a);
  auto it_b = std::ranges::begin(b);
  const auto end_a = std::ranges::end(a);
  const auto end_b = std::ranges::end(b);
  std::vector<out_t> out;

  if constexpr (std::ranges::sized_range<R1> && std::ranges::sized_range<R2>) {
    const auto na = static_cast<std::size_t>(std::ranges::size(a));
    const auto nb = static_cast<std::size_t>(std::ranges::size(b));
    out.reserve(std::min<std::size_t>(na, nb));
  }

  std::size_t i = 0;
  for (; it_a != end_a && it_b != end_b; ++it_a, ++it_b) {
    out.emplace_back(detail::invoke_binary(fn, *it_a, *it_b, i++));
  }
  return out;
}

// NumPy-like apply_along_axis for 2D nested ranges.
//
// Requirements:
// - `input` is a sized random-access range of rows
// - each row is a sized random-access range
// - rectangular (all rows same length) is assumed
//
// Semantics:
// - axis == 0: apply `fn` to each column slice (length = rows)
// - axis == 1: apply `fn` to each row slice (length = cols)
//
// The callable can accept either (std::span<const T>) or (std::span<const T>, index).
//
// NOTE: slices are materialized into a temporary std::vector<T> to keep this generic.
// If you want zero-copy views for contiguous rows, we can add a specialized fast path.

template <class F, class R2D>
requires std::ranges::random_access_range<R2D> && std::ranges::sized_range<R2D> &&
         std::ranges::random_access_range<std::ranges::range_reference_t<R2D>> &&
         std::ranges::sized_range<std::ranges::range_reference_t<R2D>>
auto apply_along_axis(F&& fn, R2D&& input, std::size_t axis, queue_options opts = {}) {
  using row_ref_t = std::ranges::range_reference_t<R2D>;
  using elem_ref_t = std::ranges::range_reference_t<row_ref_t>;
  using elem_t = std::remove_cvref_t<elem_ref_t>;

  const std::size_t nrows = static_cast<std::size_t>(std::ranges::size(input));
  const std::size_t ncols = nrows == 0 ? 0 : static_cast<std::size_t>(std::ranges::size(*(std::ranges::begin(input))));

  if (axis != 0 && axis != 1) {
    throw std::invalid_argument("apply_along_axis: axis must be 0 or 1 for 2D inputs");
  }

  // Output element type is whatever fn(span[, i]) returns.
  using slice_span_t = std::span<const elem_t>;
  using out_t = detail::invoke_result_elem_t<F&, slice_span_t>;
  std::vector<out_t> out;

  const std::size_t nslices = (axis == 0) ? ncols : nrows;
  out.reserve(nslices);

  // Parallelization is only meaningful across slices.
  if (opts.exec == queue_exec::par && opts.grain_size > 0 && std::default_initializable<out_t>) {
    out.resize(nslices);
    if (nslices == 0) return out;

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
        std::vector<elem_t> tmp;
        if (axis == 0) {
          tmp.resize(nrows);
          for (std::size_t j = start; j < end; ++j) {
            for (std::size_t i = 0; i < nrows; ++i) {
              tmp[i] = *(std::ranges::begin(input) + static_cast<std::ptrdiff_t>(i));
              tmp[i] = * (std::ranges::begin(*(std::ranges::begin(input) + static_cast<std::ptrdiff_t>(i))) + static_cast<std::ptrdiff_t>(j));
            }
            out[j] = detail::invoke_with_optional_index(fn_copy, slice_span_t(tmp.data(), tmp.size()), j);
          }
        } else {
          tmp.resize(ncols);
          for (std::size_t i = start; i < end; ++i) {
            auto row_it = std::ranges::begin(input) + static_cast<std::ptrdiff_t>(i);
            auto col_it = std::ranges::begin(*row_it);
            for (std::size_t j = 0; j < ncols; ++j) {
              tmp[j] = *(col_it + static_cast<std::ptrdiff_t>(j));
            }
            out[i] = detail::invoke_with_optional_index(fn_copy, slice_span_t(tmp.data(), tmp.size()), i);
          }
        }
      }));
    }

    for (auto& f : futures) f.get();
    return out;
  }

  std::vector<elem_t> tmp;
  if (axis == 0) {
    tmp.resize(nrows);
    for (std::size_t j = 0; j < ncols; ++j) {
      for (std::size_t i = 0; i < nrows; ++i) {
        auto row_it = std::ranges::begin(input) + static_cast<std::ptrdiff_t>(i);
        tmp[i] = *(std::ranges::begin(*row_it) + static_cast<std::ptrdiff_t>(j));
      }
      out.emplace_back(detail::invoke_with_optional_index(fn, slice_span_t(tmp.data(), tmp.size()), j));
    }
  } else {
    tmp.resize(ncols);
    for (std::size_t i = 0; i < nrows; ++i) {
      auto row_it = std::ranges::begin(input) + static_cast<std::ptrdiff_t>(i);
      auto col_it = std::ranges::begin(*row_it);
      for (std::size_t j = 0; j < ncols; ++j) {
        tmp[j] = *(col_it + static_cast<std::ptrdiff_t>(j));
      }
      out.emplace_back(detail::invoke_with_optional_index(fn, slice_span_t(tmp.data(), tmp.size()), i));
    }
  }

  return out;
}

} // namespace pluss::aux

namespace pluss {
using aux::queue;
using aux::queue_exec;
using aux::queue_into;
using aux::queue_options;
}

