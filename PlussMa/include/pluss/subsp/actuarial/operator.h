#pragma once

#include "pluss/error.h"

#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace pluss::subsp::actuarial {

template <typename Signature>
class Operator;

// A small, type-safe, type-erased actuarial operator. Domain aliases such as
// Valuation keep public signatures short while sharing this implementation.
template <typename Result, typename... Args>
class Operator<Result(Args...)> final {
public:
  Operator() = default;

  template <typename Callable>
    requires std::is_invocable_r_v<Result, Callable&, Args...>
  explicit Operator(std::string key, Callable&& callable)
      : key_(std::move(key)),
        callable_(std::forward<Callable>(callable)) {}

  [[nodiscard]] bool defined() const noexcept {
    return static_cast<bool>(callable_);
  }

  explicit operator bool() const noexcept { return defined(); }

  [[nodiscard]] std::string_view key() const noexcept { return key_; }

  [[nodiscard]] Result operator()(Args... args) const {
    PLUSS_CHECK(defined(), "actuarial Operator is undefined");
    return callable_(std::forward<Args>(args)...);
  }

private:
  std::string key_{};
  std::function<Result(Args...)> callable_{};
};

template <typename Signature, typename Callable>
[[nodiscard]] Operator<Signature> make_operator(std::string key,
                                                Callable&& callable) {
  return Operator<Signature>(std::move(key),
                             std::forward<Callable>(callable));
}

} // namespace pluss::subsp::actuarial
