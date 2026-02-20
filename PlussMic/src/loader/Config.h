#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace pluss::loader {

class ConfigValue {
public:
  using Variant = std::variant<std::monostate, bool, std::int64_t, double, std::string, std::vector<std::string>>;

  ConfigValue() = default;
  ConfigValue(std::nullptr_t) : value_(std::monostate{}) {}
  ConfigValue(bool v) : value_(v) {}
  ConfigValue(std::int32_t v) : value_(static_cast<std::int64_t>(v)) {}
  ConfigValue(std::int64_t v) : value_(v) {}
  ConfigValue(double v) : value_(v) {}
  ConfigValue(const char* v) : value_(std::string(v == nullptr ? "" : v)) {}
  ConfigValue(std::string v) : value_(std::move(v)) {}
  ConfigValue(std::string_view v) : value_(std::string(v)) {}
  ConfigValue(std::vector<std::string> v) : value_(std::move(v)) {}
  ConfigValue(std::initializer_list<std::string_view> v) {
    std::vector<std::string> out;
    out.reserve(v.size());
    for (const auto sv : v) {
      out.emplace_back(sv);
    }
    value_ = std::move(out);
  }
  ConfigValue(std::initializer_list<const char*> v) {
    std::vector<std::string> out;
    out.reserve(v.size());
    for (const auto* s : v) {
      out.emplace_back(s == nullptr ? "" : s);
    }
    value_ = std::move(out);
  }

  [[nodiscard]] bool is_none() const { return std::holds_alternative<std::monostate>(value_); }
  [[nodiscard]] bool is_bool() const { return std::holds_alternative<bool>(value_); }
  [[nodiscard]] bool is_int() const { return std::holds_alternative<std::int64_t>(value_); }
  [[nodiscard]] bool is_double() const { return std::holds_alternative<double>(value_); }
  [[nodiscard]] bool is_string() const { return std::holds_alternative<std::string>(value_); }
  [[nodiscard]] bool is_string_list() const { return std::holds_alternative<std::vector<std::string>>(value_); }

  [[nodiscard]] const Variant& raw() const { return value_; }

  [[nodiscard]] std::optional<bool> as_bool() const {
    if (const auto* v = std::get_if<bool>(&value_)) return *v;
    return std::nullopt;
  }

  [[nodiscard]] std::optional<std::int64_t> as_int() const {
    if (const auto* v = std::get_if<std::int64_t>(&value_)) return *v;
    return std::nullopt;
  }

  [[nodiscard]] std::optional<double> as_double() const {
    if (const auto* v = std::get_if<double>(&value_)) return *v;
    return std::nullopt;
  }

  [[nodiscard]] std::optional<std::string_view> as_string() const {
    if (const auto* v = std::get_if<std::string>(&value_)) return *v;
    return std::nullopt;
  }

  [[nodiscard]] std::optional<std::vector<std::string>> as_string_list() const {
    if (const auto* v = std::get_if<std::vector<std::string>>(&value_)) return *v;
    return std::nullopt;
  }

private:
  Variant value_{};
};

using Config = std::unordered_map<std::string, ConfigValue>;
using ConfigEntry = std::pair<std::string, ConfigValue>;

inline Config config_from(std::initializer_list<ConfigEntry> entries) {
  Config out;
  out.reserve(entries.size());
  for (const auto& [k, v] : entries) {
    out.emplace(k, v);
  }
  return out;
}

} // namespace pluss::loader
