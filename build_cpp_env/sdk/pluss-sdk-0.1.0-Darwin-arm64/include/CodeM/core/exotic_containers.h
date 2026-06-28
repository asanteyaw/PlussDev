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
#include "MSharp/SymInt.h"
#include "MSharp/SymBool.h"

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

using SymNode = ::pluss::msharp::SymNode;
using SymInt = ::pluss::msharp::SymInt;
using SymBool = ::pluss::msharp::SymBool;

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
