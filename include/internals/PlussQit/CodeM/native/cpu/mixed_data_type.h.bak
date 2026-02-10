#pragma once

#include <PlussQit/core/Series.h>

namespace pluss::native {

inline ScalarType first_type() {
  return ScalarType::Undefined;
}

template <typename... Args>
inline ScalarType first_type(const Series& arg, const Args&... parameters) {
  return arg.defined() ? arg.scalar_type() : first_type(parameters...);
}

template <typename... Args>
inline bool is_mixed_type(const Series& input, const Args&... parameters) {
  const auto parameter_type = first_type(parameters...);
  return ((parameter_type != ScalarType::Undefined) &&
          (parameter_type != input.scalar_type()));
}

// currently on CPU, mixed data type is only supported
// when input is 'BFloat16' or 'Half' and parameters are 'Float'
inline void check_mixed_data_type(const Series& input) {
  TORCH_CHECK(pluss::isReducedFloatingType(input.scalar_type()),
      "mixed dtype (CPU): all inputs must share same datatype.");
}

template <typename... Args>
inline void check_mixed_data_type(const Series& input, const Series& parameter, const Args&... parameters) {
  TORCH_CHECK(!parameter.defined() || parameter.scalar_type() == ScalarType::Float,
      "mixed dtype (CPU): expect parameter to have scalar type of Float");
  check_mixed_data_type(input, parameters...);
}

inline ScalarType param_scalar_type(const Series& t, bool is_mixed_type) {
  return is_mixed_type ? ScalarType::Float : t.scalar_type();
}

} // namespace pluss::native
