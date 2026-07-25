#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace pluss::ai::nodeb::io::detail {

// FNV-1a 64-bit: a small, well-known, dependency-free integrity check
// shared by every NodeB artifact format (workflow packs, embedder packs).
// The fold is inherently sequential (each byte's hash depends on the
// previous one), so there is no meaningful way to batch or vectorize it;
// artifact bodies are at most a few MB and this is a one-shot save/load
// path, not a hot loop.
[[nodiscard]] std::uint64_t fnv1a64(std::string_view data) noexcept;

[[nodiscard]] std::string to_hex(std::uint64_t value);
[[nodiscard]] std::uint64_t parse_hex_u64(std::string_view value);
[[nodiscard]] std::int64_t parse_i64(std::string_view value);

} // namespace pluss::ai::nodeb::io::detail
