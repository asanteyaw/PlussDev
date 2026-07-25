#pragma once

#include "pluss/ai/nodej/nodej.h"
#include "pluss/ai/nodej/sparse_runtime.h"
#include "pluss/codem/series_factory.h"
#include "table/DataFrame.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <functional>
#include <future>
#include <initializer_list>
#include <limits>
#include <memory>
#include <numeric>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace pluss::ai::lymph {

using namespace ::pluss::ai::nodej;


template <class T>
T resolve(StatusOr<T> value_or, std::string_view where = {}) {
  if (!value_or.ok()) {
    const std::string prefix = where.empty() ? std::string{} : (std::string(where) + ": ");
    ::pluss::throw_error(prefix + value_or.status().message);
  }
  return value_or.move_value();
}

inline void resolve(Status status, std::string_view where = {}) {
  if (status.ok()) return;
  const std::string prefix = where.empty() ? std::string{} : (std::string(where) + ": ");
  ::pluss::throw_error(prefix + status.message);
}

[[nodiscard]] inline std::vector<std::int64_t> copy_feature_route(pluss::msharp::RangeView::Int feature_route) {
  std::vector<std::int64_t> out{};
  out.reserve(feature_route.size());
  out.insert(out.end(), feature_route.begin(), feature_route.end());
  return out;
}

template <class T>
[[nodiscard]] inline std::vector<T> copy_range_values(pluss::msharp::RangeView::Of<T> values) {
  std::vector<T> out{};
  out.reserve(values.size());
  out.insert(out.end(), values.begin(), values.end());
  return out;
}

class WorkflowModel;

} // namespace pluss::ai::lymph
