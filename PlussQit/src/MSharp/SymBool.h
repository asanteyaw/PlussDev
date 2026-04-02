#pragma once

#include <string>
#include <string_view>

#include "CodeM/core/Errors.h"

namespace pluss::msharp {

struct SymNode;

// Torch-inspired symbolic boolean.
//
// Current behavior:
// - Concrete SymBool stores a bool.
// - Symbolic SymBool stores an opaque, non-owning node pointer.
// - Materializing symbolic value throws until a symbolic boolean engine exists.
class SymBool {
public:
  constexpr SymBool() noexcept = default;
  constexpr SymBool(bool v) noexcept : kind_(Kind::Concrete) { u_.value = v; }
  explicit constexpr SymBool(const SymNode* n) noexcept : kind_(Kind::Symbolic) { u_.node = n; }

  [[nodiscard]] constexpr bool is_symbolic() const noexcept { return kind_ == Kind::Symbolic; }
  [[nodiscard]] constexpr bool is_concrete() const noexcept { return kind_ == Kind::Concrete; }

  [[nodiscard]] bool expect_bool(std::string_view where = {}) const {
    if (is_concrete()) return u_.value;
    ::pluss::throw_error(
        where.empty()
            ? "SymBool: attempted to materialize a symbolic value"
            : (std::string(where) + ": attempted to materialize a symbolic SymBool"));
  }

  [[nodiscard]] constexpr bool unsafe_bool() const noexcept { return u_.value; }
  [[nodiscard]] constexpr const SymNode* node() const noexcept { return u_.node; }

  friend constexpr bool operator==(SymBool a, SymBool b) noexcept {
    if (a.is_concrete() && b.is_concrete()) return a.u_.value == b.u_.value;
    return a.u_.node == b.u_.node;
  }
  friend constexpr bool operator!=(SymBool a, SymBool b) noexcept { return !(a == b); }

  friend constexpr SymBool operator!(SymBool x) noexcept {
    if (x.is_concrete()) return SymBool{!x.u_.value};
    return SymBool{static_cast<const SymNode*>(nullptr)};
  }
  friend constexpr SymBool operator&&(SymBool a, SymBool b) noexcept {
    if (a.is_concrete() && b.is_concrete()) return SymBool{a.u_.value && b.u_.value};
    return SymBool{static_cast<const SymNode*>(nullptr)};
  }
  friend constexpr SymBool operator||(SymBool a, SymBool b) noexcept {
    if (a.is_concrete() && b.is_concrete()) return SymBool{a.u_.value || b.u_.value};
    return SymBool{static_cast<const SymNode*>(nullptr)};
  }

private:
  enum class Kind : unsigned char { Concrete, Symbolic };
  Kind kind_{Kind::Concrete};

  union U {
    constexpr U() : value(false) {}
    bool value;
    const SymNode* node;
  } u_{};
};

} // namespace pluss::msharp

