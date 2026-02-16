#pragma once
#include "pluss/export.h"
#include <cstdint>
#include <variant>

namespace pluss::codem {

// Minimal scalar container (CPU-only).
class CODEM_API Scalar final {
public:
  using Storage = std::variant<bool, std::int64_t, std::uint64_t, double>;
  Scalar(bool v) : v_(v) {}
  Scalar(std::int64_t v) : v_(v) {}
  Scalar(std::uint64_t v) : v_(v) {}
  Scalar(double v) : v_(v) {}

  template <class T>
  T to() const { return std::get<T>(v_); }

  const Storage& storage() const noexcept { return v_; }

private:
  Storage v_;
};

} // namespace pluss::codem
