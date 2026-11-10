#pragma once

// `suppose("premise", pluss_value, predicate)` checks a named premise,
// derivations, and a conclusion for a concrete Pluss value. Value pipelines
// use `flow(value)`; `SupposeBox` remains as an alias for older source code.

#include <concepts>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::codem { class Series; }
namespace pluss::coden { class Note; }
namespace pluss::codeh { class Timeline; class Duration; }
namespace pluss::codev { class Queue; }
namespace pluss::table { class Slate; class DataFrame; class Atlas; }
namespace pluss::kindle { class Stream; class Catalog; class Archive; }
namespace pluss::scribble { class Slot; class Registry; class Axel; }
namespace pluss::bundle { class Locus; class Nexus; }

namespace pluss::tils {

template <class T>
inline constexpr bool enable_suppose = false;

template <> inline constexpr bool enable_suppose<codem::Series> = true;
template <> inline constexpr bool enable_suppose<coden::Note> = true;
template <> inline constexpr bool enable_suppose<codeh::Timeline> = true;
template <> inline constexpr bool enable_suppose<codeh::Duration> = true;
template <> inline constexpr bool enable_suppose<codev::Queue> = true;
template <> inline constexpr bool enable_suppose<table::Slate> = true;
template <> inline constexpr bool enable_suppose<table::DataFrame> = true;
template <> inline constexpr bool enable_suppose<table::Atlas> = true;
template <> inline constexpr bool enable_suppose<kindle::Stream> = true;
template <> inline constexpr bool enable_suppose<kindle::Catalog> = true;
template <> inline constexpr bool enable_suppose<kindle::Archive> = true;
template <> inline constexpr bool enable_suppose<scribble::Slot> = true;
template <> inline constexpr bool enable_suppose<scribble::Registry> = true;
template <> inline constexpr bool enable_suppose<scribble::Axel> = true;
template <> inline constexpr bool enable_suppose<bundle::Locus> = true;
template <> inline constexpr bool enable_suppose<bundle::Nexus> = true;

template <class T>
concept supposition_value = enable_suppose<std::remove_cvref_t<T>>;

template <class T>
class FlowBox {
 public:
  using value_type = T;

  constexpr explicit FlowBox(T value) : value_(std::move(value)) {}

  constexpr T& value() & noexcept { return value_; }
  constexpr const T& value() const& noexcept { return value_; }
  constexpr T&& value() && noexcept { return std::move(value_); }
  constexpr const T&& value() const&& noexcept { return std::move(value_); }

  constexpr T& unwrap() & noexcept { return value_; }
  constexpr const T& unwrap() const& noexcept { return value_; }
  constexpr T&& unwrap() && noexcept { return std::move(value_); }
  constexpr const T&& unwrap() const&& noexcept { return std::move(value_); }

  template <class F>
  constexpr auto then(F&& fn) & {
    return FlowBox<std::decay_t<std::invoke_result_t<F&, T&>>>(
        std::invoke(std::forward<F>(fn), value_));
  }

  template <class F>
  constexpr auto then(F&& fn) const& {
    return FlowBox<std::decay_t<std::invoke_result_t<F&, const T&>>>(
        std::invoke(std::forward<F>(fn), value_));
  }

  template <class F>
  constexpr auto then(F&& fn) && {
    return FlowBox<std::decay_t<std::invoke_result_t<F&, T&&>>>(
        std::invoke(std::forward<F>(fn), std::move(value_)));
  }

  template <class F>
  constexpr auto transform(F&& fn) & { return then(std::forward<F>(fn)); }

  template <class F>
  constexpr auto transform(F&& fn) const& { return then(std::forward<F>(fn)); }

  template <class F>
  constexpr auto transform(F&& fn) && { return std::move(*this).then(std::forward<F>(fn)); }

  template <class F>
  constexpr FlowBox& tap(F&& fn) & {
    std::invoke(std::forward<F>(fn), value_);
    return *this;
  }

  template <class F>
  constexpr const FlowBox& tap(F&& fn) const& {
    std::invoke(std::forward<F>(fn), value_);
    return *this;
  }

  template <class F>
  constexpr FlowBox&& tap(F&& fn) && {
    std::invoke(std::forward<F>(fn), value_);
    return std::move(*this);
  }

  template <class U>
  constexpr auto into() const& {
    static_assert(std::is_constructible_v<U, const T&>,
                  "flow::into<U>: U must be constructible from const T&");
    return FlowBox<U>(U(value_));
  }

  template <class U>
  constexpr auto into() && {
    static_assert(std::is_constructible_v<U, T&&>,
                  "flow::into<U>: U must be constructible from T&&");
    return FlowBox<U>(U(std::move(value_)));
  }

 private:
  T value_;
};

template <class T>
using SupposeBox = FlowBox<T>;

enum class SuppositionStatus {
  verified,
  premise_not_met,
  derivation_failed,
  conclusion_failed,
};

enum class SuppositionStepKind { premise, derivation, conclusion };
enum class SuppositionStepStatus { passed, failed, skipped };

struct SuppositionStep {
  std::string name;
  SuppositionStepKind kind;
  SuppositionStepStatus status;
};

class SuppositionResult {
 public:
  SuppositionResult(SuppositionStatus status, std::vector<SuppositionStep> steps)
      : status_(status), steps_(std::move(steps)) {}

  [[nodiscard]] SuppositionStatus status() const noexcept { return status_; }
  [[nodiscard]] bool verified() const noexcept {
    return status_ == SuppositionStatus::verified;
  }
  [[nodiscard]] bool applicable() const noexcept {
    return status_ != SuppositionStatus::premise_not_met;
  }
  [[nodiscard]] const std::vector<SuppositionStep>& steps() const noexcept {
    return steps_;
  }
  [[nodiscard]] std::string_view failed_step() const noexcept {
    for (const auto& step : steps_) {
      if (step.status == SuppositionStepStatus::failed) return step.name;
    }
    return {};
  }

 private:
  SuppositionStatus status_;
  std::vector<SuppositionStep> steps_;
};

template <class T>
class Supposition {
 public:
  using value_type = std::remove_cvref_t<T>;

  template <class U, class Predicate>
    requires std::predicate<Predicate&, const value_type&>
  explicit Supposition(std::string name, U&& value, Predicate&& check)
      : value_(std::forward<U>(value)) {
    if (name.empty()) throw std::invalid_argument("suppose: premise name must not be empty");
    steps_.reserve(4);
    const bool condition = static_cast<bool>(std::invoke(std::forward<Predicate>(check), value_));
    status_ = condition ? SuppositionStatus::verified : SuppositionStatus::premise_not_met;
    steps_.push_back({std::move(name), SuppositionStepKind::premise,
                      condition ? SuppositionStepStatus::passed : SuppositionStepStatus::failed});
  }

  [[nodiscard]] const value_type& value() const noexcept { return value_; }

  template <class Predicate>
    requires std::predicate<Predicate&, const value_type&>
  Supposition& then(std::string_view name, Predicate&& check) & {
    add_derivation(name, std::forward<Predicate>(check));
    return *this;
  }

  template <class Predicate>
    requires std::predicate<Predicate&, const value_type&>
  Supposition&& then(std::string_view name, Predicate&& check) && {
    add_derivation(name, std::forward<Predicate>(check));
    return std::move(*this);
  }

  template <class Predicate>
    requires std::predicate<Predicate&, const value_type&>
  [[nodiscard]] SuppositionResult conclude(std::string_view name, Predicate&& check) const& {
    auto steps = steps_;
    return finish(status_, std::move(steps), name, std::forward<Predicate>(check));
  }

  template <class Predicate>
    requires std::predicate<Predicate&, const value_type&>
  [[nodiscard]] SuppositionResult conclude(std::string_view name, Predicate&& check) && {
    return finish(status_, std::move(steps_), name, std::forward<Predicate>(check));
  }

 private:
  template <class Predicate>
  void add_derivation(std::string_view name, Predicate&& check) {
    if (name.empty()) throw std::invalid_argument("suppose.then: step name must not be empty");
    if (status_ != SuppositionStatus::verified) {
      steps_.push_back({std::string(name), SuppositionStepKind::derivation,
                        SuppositionStepStatus::skipped});
      return;
    }
    const bool passed = static_cast<bool>(std::invoke(std::forward<Predicate>(check), value_));
    steps_.push_back({std::string(name), SuppositionStepKind::derivation,
                      passed ? SuppositionStepStatus::passed : SuppositionStepStatus::failed});
    if (!passed) status_ = SuppositionStatus::derivation_failed;
  }

  template <class Predicate>
  SuppositionResult finish(SuppositionStatus status,
                           std::vector<SuppositionStep> steps,
                           std::string_view name, Predicate&& check) const {
    if (name.empty()) throw std::invalid_argument("suppose.conclude: conclusion name must not be empty");
    if (status != SuppositionStatus::verified) {
      steps.push_back({std::string(name), SuppositionStepKind::conclusion,
                       SuppositionStepStatus::skipped});
      return {status, std::move(steps)};
    }
    const bool passed = static_cast<bool>(std::invoke(std::forward<Predicate>(check), value_));
    steps.push_back({std::string(name), SuppositionStepKind::conclusion,
                     passed ? SuppositionStepStatus::passed : SuppositionStepStatus::failed});
    return {passed ? SuppositionStatus::verified : SuppositionStatus::conclusion_failed,
            std::move(steps)};
  }

  T value_;
  SuppositionStatus status_;
  std::vector<SuppositionStep> steps_;
};

struct FlowFn {
  template <class T>
  constexpr auto operator()(T&& value) const {
    return FlowBox<std::decay_t<T>>(std::forward<T>(value));
  }

  template <class To, class From>
  constexpr auto as(From&& value) const {
    static_assert(std::is_constructible_v<To, From&&>,
                  "flow.as<To>(value): To must be constructible from value");
    return FlowBox<To>(To(std::forward<From>(value)));
  }
};

struct SupposeFn {
  template <supposition_value T, class Predicate>
    requires std::predicate<Predicate&, const std::remove_cvref_t<T>&>
  [[nodiscard]] auto operator()(std::string_view name, T&& value, Predicate&& check) const {
    using Stored = std::conditional_t<std::is_lvalue_reference_v<T>,
                                      const std::remove_reference_t<T>&, std::remove_cvref_t<T>>;
    return Supposition<Stored>(std::string(name), std::forward<T>(value),
                               std::forward<Predicate>(check));
  }
};

inline constexpr FlowFn flow{};
inline constexpr SupposeFn suppose{};

}  // namespace pluss::tils
