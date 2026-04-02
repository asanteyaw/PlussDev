#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include "CodeM/core/ScalarType.h"

//
// Torch-inspired lightweight container/value types used by the generated Series API.
//
// IMPORTANT DESIGN GOALS
// - Header-only and dependency-light (safe to include from public headers)
// - ABI-stable-ish: prefer small trivially-copyable wrapper types
// - C++20-friendly (std::span, std::optional, std::string_view)
// - Keep reference-style list inputs lightweight and non-owning
//

namespace pluss::codem {

// ----------------------------------------------------------------------------
// Torch-ish `optional` alias (codegen expects `optional<T>` in this namespace).
// ----------------------------------------------------------------------------
template <class T>
using optional = std::optional<T>;

// ----------------------------------------------------------------------------
// ArrayRef (generic): we keep this as `std::span<const T>` because it is already
// ubiquitous in the project.
// ----------------------------------------------------------------------------
template <class T>
using ArrayRef = std::span<const T>;

// ----------------------------------------------------------------------------
// SymInt
// ----------------------------------------------------------------------------
// Torch-inspired symbolic integer.
//
// PlussQit does not currently maintain a symbolic shape graph, but we keep a
// forward-compatible representation so the public API can evolve without churn.
//
// This is NOT a copy of PyTorch's SymInt — it is a small, modern C++20 design
// that keeps the door open for later symbolic support.
//
// - Concrete SymInt stores an int64.
// - Symbolic SymInt stores an opaque, non-owning node pointer.
// - Attempting to materialize a symbolic SymInt throws (until a shape engine
//   is introduced).
//
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
    throw std::runtime_error(
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

  // Concrete arithmetic; symbolic propagates "unknown" (null node).
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

// ----------------------------------------------------------------------------
// Torch-aligned placeholder types (progressively replaced as backends grow).
// ----------------------------------------------------------------------------
enum class Layout : std::uint8_t {
  Strided = 0,
  Sparse = 1,
  SparseCsr = 2,
  SparseCsc = 3,
  SparseBsr = 4,
  SparseBsc = 5
};

struct Dimname {
  std::string value;
};

using DimnameList = std::vector<Dimname>;

// Generator placeholder (deterministic RNG lives elsewhere for now).
struct Generator {
  std::uint64_t seed = 0;
};

enum class MemoryFormat : std::uint8_t {
  Contiguous = 0,
  Preserve = 1
};

struct Device {
  enum class Type : std::uint8_t { CPU = 0, CUDA = 1 };
  Type type = Type::CPU;
  int index = -1;
};

// Options placeholder. Once you support devices/layouts/etc, move these to a
// dedicated Options header to avoid circular includes.
struct SeriesOptions {
  optional<ScalarType> dtype;
  optional<Layout> layout;
  optional<Device> device;
  optional<bool> pin_memory;
};

} // namespace pluss::codem
