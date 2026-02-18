#pragma once

#include <concepts>
#include <cstddef>
#include <future>
#include <functional>
#include <iterator>
#include <ranges>
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

} // namespace pluss::aux

namespace pluss {
using aux::queue;
using aux::queue_exec;
using aux::queue_into;
using aux::queue_options;
}

