#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "CodeM/core/Series.h"
#include "CodeM/Traverse/TraverseFwd.h"
#include "pluss/export.h"

namespace pluss {
template <class T>
class StackArray;
} // namespace pluss

namespace pluss::codem {

class CODEM_API TraverseMatch final {
public:
  TraverseMatch() = default;
  TraverseMatch(Series coords, Series values, std::vector<std::int64_t> source_shape, std::int64_t indexed_rank);
  TraverseMatch(Series coords, std::vector<std::int64_t> source_shape, std::int64_t indexed_rank);

  bool defined() const noexcept;
  std::int64_t count() const;
  std::int64_t rank() const noexcept;
  std::int64_t indexed_rank() const noexcept;
  std::span<const std::int64_t> source_shape() const noexcept;

  bool has_values() const noexcept;

  const Series& coords() const noexcept;
  const Series& values() const noexcept;

  ::pluss::StackArray<Series> split_indices() const;
  Series linear_indices() const;
  Series select_from(const Series& other) const;

private:
  Series coords_{pluss::series::undefined()};
  Series values_{pluss::series::undefined()};
  std::vector<std::int64_t> source_shape_{};
  std::int64_t indexed_rank_{0};
};

} // namespace pluss::codem
