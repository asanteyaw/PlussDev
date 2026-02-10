#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>

#include <MSharp/core/ScalarType.h>

namespace caffe2 {
struct TypeMeta {
  // Minimal stub: real implementation lives in upstream caffe2/ATen.
  static TypeMeta fromScalarType(::MSharp::ScalarType) {
    return TypeMeta{};
  }
  ::MSharp::ScalarType toScalarType() const { return ::MSharp::ScalarType::Undefined; }
  bool isScalarType(::MSharp::ScalarType) const { return false; }
};
} // namespace caffe2

