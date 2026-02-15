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
// - Avoid overload collisions with std::span by using wrapper reference types
//

namespace pluss::codem {

// ----------------------------------------------------------------------------
// Torch-ish `optional` alias (codegen expects `optional<T>` in this namespace).
// ----------------------------------------------------------------------------
template <class T>
using optional = std::optional<T>;

// ----------------------------------------------------------------------------
// ArrayRef (generic): we keep this as `std::span<const T>` because it is already
// ubiquitous in the project. For IntArrayRef we intentionally use a wrapper
// type (below) to avoid overload collisions where both IntArrayRef and
// `std::span<const int64_t>` overloads exist.
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
// IntArrayRef / SymIntArrayRef
// ----------------------------------------------------------------------------
// Torch uses `at::IntArrayRef` (ArrayRef<int64_t>) which is a distinct type
// from spans/vectors; we do the same to avoid overload collisions in the codegen.
//
// The wrapper is tiny (one pointer + one size) and trivially copyable.
// It is implicitly constructible from std::span / vector / initializer_list.
// ----------------------------------------------------------------------------
class IntArrayRef {
public:
  using value_type = std::int64_t;

  constexpr IntArrayRef() noexcept = default;
  constexpr IntArrayRef(const std::int64_t* data, std::size_t size) noexcept : span_(data, size) {}
  constexpr IntArrayRef(std::span<const std::int64_t> s) noexcept : span_(s) {}

  IntArrayRef(const std::vector<std::int64_t>& v) noexcept : span_(v.data(), v.size()) {}
  IntArrayRef(std::initializer_list<std::int64_t> il) noexcept : span_(il.begin(), il.size()) {}

  [[nodiscard]] constexpr const std::int64_t* data() const noexcept { return span_.data(); }
  [[nodiscard]] constexpr std::size_t size() const noexcept { return span_.size(); }
  [[nodiscard]] constexpr bool empty() const noexcept { return span_.empty(); }

  [[nodiscard]] constexpr const std::int64_t& operator[](std::size_t i) const noexcept { return span_[i]; }

  [[nodiscard]] constexpr auto begin() const noexcept { return span_.begin(); }
  [[nodiscard]] constexpr auto end() const noexcept { return span_.end(); }

  [[nodiscard]] constexpr std::span<const std::int64_t> span() const noexcept { return span_; }
  constexpr operator std::span<const std::int64_t>() const noexcept { return span_; }

private:
  std::span<const std::int64_t> span_{};
};

class SymIntArrayRef {
public:
  using value_type = SymInt;

  constexpr SymIntArrayRef() noexcept = default;
  constexpr SymIntArrayRef(const SymInt* data, std::size_t size) noexcept : span_(data, size) {}
  constexpr SymIntArrayRef(std::span<const SymInt> s) noexcept : span_(s) {}

  SymIntArrayRef(const std::vector<SymInt>& v) noexcept : span_(v.data(), v.size()) {}
  SymIntArrayRef(std::initializer_list<SymInt> il) noexcept : span_(il.begin(), il.size()) {}

  [[nodiscard]] constexpr const SymInt* data() const noexcept { return span_.data(); }
  [[nodiscard]] constexpr std::size_t size() const noexcept { return span_.size(); }
  [[nodiscard]] constexpr bool empty() const noexcept { return span_.empty(); }

  [[nodiscard]] constexpr const SymInt& operator[](std::size_t i) const noexcept { return span_[i]; }

  [[nodiscard]] constexpr auto begin() const noexcept { return span_.begin(); }
  [[nodiscard]] constexpr auto end() const noexcept { return span_.end(); }

  [[nodiscard]] constexpr std::span<const SymInt> span() const noexcept { return span_; }
  constexpr operator std::span<const SymInt>() const noexcept { return span_; }

private:
  std::span<const SymInt> span_{};
};

using OptionalIntArrayRef = std::optional<IntArrayRef>;
using OptionalSymIntArrayRef = std::optional<SymIntArrayRef>;

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
