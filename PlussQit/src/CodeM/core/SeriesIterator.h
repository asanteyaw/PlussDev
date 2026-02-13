#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <functional>

#include "CodeM/core/Series.h"

namespace pluss::codem {

// ATen-inspired SeriesIterator for CPU.
// Features:
// - broadcasting (torch-like)
// - expanded strides (0 for broadcast dims)
// - basic dimension coalescing for faster linear iteration
// - optional parallel for_each_index when numel is large
class SeriesIterator final {
public:
  static SeriesIterator binary(const Series& a, const Series& b, const Series& out);
  static SeriesIterator unary(const Series& a, const Series& out);
  static SeriesIterator ternary(const Series& m, const Series& a, const Series& b, const Series& out);

  std::span<const std::int64_t> shape() const noexcept { return shape_; }
  std::int64_t numel() const noexcept;

  const Series& m() const noexcept { return m_; }
  const Series& a() const noexcept { return a_; }
  const Series& b() const noexcept { return b_; }
  const Series& out() const noexcept { return out_; }

  void copy_out_from_in(const Series& in, Series& out) const;

  // Parallel or sequential index traversal. Callback receives:
  // (linear_index, a_offset_elems, b_offset_elems, out_offset_elems)
  void for_each_index(const std::function<void(std::int64_t, std::int64_t, std::int64_t, std::int64_t)>& fn) const;
  // (linear_index, m_off, a_off, b_off, out_off)
  void for_each_index_ternary(const std::function<void(std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t)>& fn) const;

  static std::vector<std::int64_t> broadcast_shape(std::span<const std::int64_t> a,
                                                  std::span<const std::int64_t> b);

private:
  Series m_{}, a_{}, b_{}, out_{};
  std::vector<std::int64_t> shape_;
  std::vector<std::int64_t> m_strides_, a_strides_, b_strides_, o_strides_; // in elements

  static std::vector<std::int64_t> expand_strides(std::span<const std::int64_t> sizes,
                                                  std::span<const std::int64_t> strides,
                                                  std::span<const std::int64_t> out_shape);

  void coalesce_dims(); // simple coalescing for contiguous-ish dims
};

} // namespace pluss::codem
