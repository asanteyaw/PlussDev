#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace pluss::tils::env {

// Returns nullopt when the variable is unset or the key is empty.
[[nodiscard]] std::optional<std::string> get(std::string_view key);

// Returns fallback when the variable is unset or the key is empty.
[[nodiscard]] std::string get_or(std::string_view key, std::string fallback = {});

// Parses common bool tokens:
// true-set: 1, true, yes, on
// false-set: 0, false, no, off
// Falls back when unset or invalid.
[[nodiscard]] bool get_bool(std::string_view key, bool fallback = false);

// Parses a strict integer token; falls back when unset or invalid.
[[nodiscard]] int get_int(std::string_view key, int fallback);

// Parses a strict floating token; falls back when unset or invalid.
[[nodiscard]] double get_double(std::string_view key, double fallback);

// Typed env readers with explicit clamp bounds.
[[nodiscard]] int get_int_clamped(
    std::string_view key, int fallback, int min_value, int max_value);
[[nodiscard]] double get_double_clamped(
    std::string_view key, double fallback, double min_value, double max_value);

} // namespace pluss::tils::env
