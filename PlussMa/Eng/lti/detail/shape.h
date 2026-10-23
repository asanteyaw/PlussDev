#pragma once

#include "CodeM/core/Errors.h"
#include "pluss/codem/codem.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::subsp::eng::lti::detail {

inline std::vector<std::int64_t> shape(const Series &value) {
  return {value.sizes().begin(), value.sizes().end()};
}

inline std::vector<std::int64_t> batch_shape(const Series &value,
                                             std::size_t tail) {
  auto result = shape(value);
  PLUSS_CHECK(result.size() >= tail,
              "eng::lti: value rank is smaller than its structural tail");
  result.resize(result.size() - tail);
  return result;
}

inline std::vector<std::int64_t>
broadcast(const std::vector<std::int64_t> &left,
          const std::vector<std::int64_t> &right, std::string_view operation) {
  const std::size_t rank = std::max(left.size(), right.size());
  std::vector<std::int64_t> result(rank, 1);
  for (std::size_t offset = 0; offset < rank; ++offset) {
    const auto l = offset < left.size() ? left[left.size() - 1 - offset] : 1;
    const auto r = offset < right.size() ? right[right.size() - 1 - offset] : 1;
    PLUSS_CHECK(l == r || l == 1 || r == 1,
                std::string(operation) + ": batch dimensions do not broadcast");
    result[rank - 1 - offset] = std::max(l, r);
  }
  return result;
}

inline Series expand_batch(const Series &value,
                           const std::vector<std::int64_t> &batch,
                           std::size_t tail) {
  auto source = shape(value);
  const std::size_t target_rank = batch.size() + tail;
  if (source.size() < target_rank)
    source.insert(source.begin(), target_rank - source.size(), 1);
  Series aligned = source.size() == static_cast<std::size_t>(value.dim())
                       ? value
                       : value.reshape(source);
  std::vector<std::int64_t> target = batch;
  const auto original = shape(value);
  target.insert(target.end(),
                original.end() - static_cast<std::ptrdiff_t>(tail),
                original.end());
  return aligned.expand(target, true);
}

} // namespace pluss::subsp::eng::lti::detail
