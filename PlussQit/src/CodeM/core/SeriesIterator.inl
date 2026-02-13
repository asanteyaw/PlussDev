#pragma once
#include <vector>

namespace pluss::codem {

template <class Fn>
void SeriesIterator::for_each_index(Fn&& fn) const {
  const auto n = numel();
  std::vector<std::int64_t> idx(shape_.size(), 0);

  auto linear_to_index = [&](std::int64_t lin) {
    for (std::int64_t i = static_cast<std::int64_t>(shape_.size()) - 1; i >= 0; --i) {
      const auto s = shape_[static_cast<std::size_t>(i)];
      idx[static_cast<std::size_t>(i)] = (s == 0) ? 0 : (lin % s);
      lin = (s == 0) ? 0 : (lin / s);
    }
  };

  auto offset = [&](const std::vector<std::int64_t>& strides) -> std::int64_t {
    std::int64_t off = 0;
    for (std::size_t i = 0; i < idx.size(); ++i) off += idx[i] * strides[i];
    return off;
  };

  for (std::int64_t lin = 0; lin < n; ++lin) {
    linear_to_index(lin);
    fn(lin, offset(a_strides_), offset(b_strides_), offset(o_strides_));
  }
}

} // namespace pluss::codem
