#pragma once

#include <string>

namespace pluss {
class Series;
} // namespace pluss

namespace MSharp {
struct IValue;
namespace detail {
// Determine the return type of `IValue::to() const &`. It's a const
// reference when possible and a copy otherwise. It is in this
// separate header so that List can use it as well.
template<typename T>
struct ivalue_to_const_ref_overload_return {
  using type = T;
};

template<>
struct ivalue_to_const_ref_overload_return<pluss::Series> {
  using type = const pluss::Series&;
};

template<>
struct ivalue_to_const_ref_overload_return<std::string> {
  using type = const std::string&;
};

template<>
struct ivalue_to_const_ref_overload_return<IValue> {
  using type = const IValue&;
};

} // namespace detail
} // namespace MSharp
