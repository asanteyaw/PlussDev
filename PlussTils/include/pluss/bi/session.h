#pragma once

#include "pluss/bi/specs.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace pluss::bi {

using ParamValue = std::variant<double, std::string>;

class Session {
public:
  Session() = default;

  [[nodiscard]] bool empty() const noexcept {
    return filters_.empty() && params_.empty();
  }

  [[nodiscard]] std::uint64_t version() const noexcept {
    return version_;
  }

  [[nodiscard]] bool has_filter(std::string_view key) const {
    return filters_.find(std::string(key)) != filters_.end();
  }

  [[nodiscard]] std::optional<Filter> filter(std::string_view key) const {
    const auto it = filters_.find(std::string(key));
    if (it == filters_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  [[nodiscard]] const std::map<std::string, Filter>& filters() const noexcept {
    return filters_;
  }

  Session& set_filter(Filter value) {
    PLUSS_CHECK(!value.key.empty(), "pluss::bi::Session::set_filter: filter key cannot be empty");
    PLUSS_CHECK(!value.feature.empty(), "pluss::bi::Session::set_filter: filter feature cannot be empty");
    filters_[value.key] = std::move(value);
    ++version_;
    return *this;
  }

  Session& clear_filter(std::string_view key) {
    filters_.erase(std::string(key));
    ++version_;
    return *this;
  }

  Session& clear_filters() {
    filters_.clear();
    ++version_;
    return *this;
  }

  [[nodiscard]] bool has_param(std::string_view key) const {
    return params_.find(std::string(key)) != params_.end();
  }

  [[nodiscard]] std::optional<ParamValue> param(std::string_view key) const {
    const auto it = params_.find(std::string(key));
    if (it == params_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  [[nodiscard]] const std::map<std::string, ParamValue>& params() const noexcept {
    return params_;
  }

  Session& set_param(std::string_view key, double value) {
    PLUSS_CHECK(!key.empty(), "pluss::bi::Session::set_param: parameter key cannot be empty");
    params_[std::string(key)] = value;
    ++version_;
    return *this;
  }

  Session& set_param(std::string_view key, std::string_view value) {
    PLUSS_CHECK(!key.empty(), "pluss::bi::Session::set_param: parameter key cannot be empty");
    params_[std::string(key)] = std::string(value);
    ++version_;
    return *this;
  }

  Session& clear_param(std::string_view key) {
    params_.erase(std::string(key));
    ++version_;
    return *this;
  }

  Session& clear_params() {
    params_.clear();
    ++version_;
    return *this;
  }

private:
  std::map<std::string, Filter> filters_{};
  std::map<std::string, ParamValue> params_{};
  std::uint64_t version_{0};
};

} // namespace pluss::bi
