#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include "CodeM/core/Errors.h"

namespace pluss::msharp {

// Torch-inspired symbolic integer.
//
// Current behavior:
// - Concrete SymInt stores an int64.
// - Symbolic SymInt stores an opaque, non-owning node pointer.
// - Materializing symbolic value throws until a symbolic shape engine exists.
struct SymNode;

class SymInt {
public:
  constexpr SymInt() noexcept = default;
  constexpr SymInt(std::int64_t v) noexcept : kind_(Kind::Concrete) { u_.value = v; }
  explicit constexpr SymInt(const SymNode* n) noexcept : kind_(Kind::Symbolic) { u_.node = n; }

  [[nodiscard]] constexpr bool is_symbolic() const noexcept { return kind_ == Kind::Symbolic; }
  [[nodiscard]] constexpr bool is_concrete() const noexcept { return kind_ == Kind::Concrete; }

  [[nodiscard]] std::int64_t expect_int(std::string_view where = {}) const {
    if (is_concrete()) return u_.value;
    ::pluss::throw_error(
        where.empty()
            ? "SymInt: attempted to materialize a symbolic value"
            : (std::string(where) + ": attempted to materialize a symbolic SymInt"));
  }

  [[nodiscard]] constexpr std::int64_t unsafe_int() const noexcept { return u_.value; }
  [[nodiscard]] constexpr const SymNode* node() const noexcept { return u_.node; }

  friend constexpr bool operator==(SymInt a, SymInt b) noexcept {
    if (a.is_concrete() && b.is_concrete()) return a.u_.value == b.u_.value;
    return a.u_.node == b.u_.node;
  }
  friend constexpr bool operator!=(SymInt a, SymInt b) noexcept { return !(a == b); }

  friend constexpr SymInt operator+(SymInt a, SymInt b) noexcept {
    if (a.is_concrete() && b.is_concrete()) return SymInt{a.u_.value + b.u_.value};
    return SymInt{static_cast<const SymNode*>(nullptr)};
  }
  friend constexpr SymInt operator-(SymInt a, SymInt b) noexcept {
    if (a.is_concrete() && b.is_concrete()) return SymInt{a.u_.value - b.u_.value};
    return SymInt{static_cast<const SymNode*>(nullptr)};
  }
  friend constexpr SymInt operator*(SymInt a, SymInt b) noexcept {
    if (a.is_concrete() && b.is_concrete()) return SymInt{a.u_.value * b.u_.value};
    return SymInt{static_cast<const SymNode*>(nullptr)};
  }

private:
  enum class Kind : std::uint8_t { Concrete, Symbolic };
  Kind kind_{Kind::Concrete};

  union U {
    constexpr U() : value(0) {}
    std::int64_t value;
    const SymNode* node;
  } u_{};
};

} // namespace pluss::msharp

