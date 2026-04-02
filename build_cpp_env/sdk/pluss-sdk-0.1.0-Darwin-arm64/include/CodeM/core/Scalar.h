#pragma once
#include "pluss/export.h"
#include "CodeM/core/ScalarType.h"
#include <cstdint>
#include <ostream>
#include <variant>

namespace pluss::codem {

// Minimal scalar container (CPU-only).
class CODEM_API Scalar final {
public:
  using Storage = std::variant<bool, std::int64_t, std::uint64_t, double>;
  Scalar(bool v, ScalarType dtype = ScalarType::Bool) : v_(v), dtype_(dtype) {}
  Scalar(std::int64_t v, ScalarType dtype = ScalarType::Int64) : v_(v), dtype_(dtype) {}
  Scalar(std::uint64_t v, ScalarType dtype = ScalarType::UInt64) : v_(v), dtype_(dtype) {}
  Scalar(double v, ScalarType dtype = ScalarType::Float64) : v_(v), dtype_(dtype) {}

  template <class T>
  T to() const { return std::get<T>(v_); }

  ScalarType dtype() const noexcept { return dtype_; }
  const Storage& storage() const noexcept { return v_; }

private:
  Storage v_;
  ScalarType dtype_{ScalarType::Float64};
};

inline std::ostream& operator<<(std::ostream& os, const Scalar& s) {
  os << "Scalar(dtype=" << s.dtype() << ", value=";
  std::visit([&](const auto& v) { os << v; }, s.storage());
  os << ")";
  return os;
}

} // namespace pluss::codem
