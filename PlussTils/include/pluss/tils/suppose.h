#pragma once

// `suppose("premise", pluss_value, predicate)` checks a named premise,
// optional typed derivations, and a conclusion for a concrete Pluss value.
// Unchecked value pipelines use `flow(value)`.

#include <concepts>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
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

enum class SuppositionStatus {
  verified,
  premise_not_met,
  derivation_failed,
  conclusion_failed,
};

enum class SuppositionStepKind { premise, derivation, check, conclusion };
enum class SuppositionStepStatus { computed, passed, failed, skipped };

struct SuppositionStep {
  std::string name;
  SuppositionStepKind kind;
  SuppositionStepStatus status;
};

template <class... Facts>
class SuppositionResult {
 public:
  SuppositionResult(SuppositionStatus status, std::vector<SuppositionStep> steps,
                    std::tuple<std::optional<Facts>...> facts)
      : status_(status), steps_(std::move(steps)), facts_(std::move(facts)) {}

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

  template <std::size_t Index>
  [[nodiscard]] const auto& derived() const noexcept {
    return std::get<Index>(facts_);
  }

 private:
  SuppositionStatus status_;
  std::vector<SuppositionStep> steps_;
  std::tuple<std::optional<Facts>...> facts_;
};

template <class T, class... Facts>
class Supposition {
 public:
  using value_type = std::remove_cvref_t<T>;

  template <class U, class Predicate>
    requires (sizeof...(Facts) == 0 && std::predicate<Predicate&, const value_type&>)
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

  template <class Derive>
    requires std::invocable<Derive&, const value_type&, const Facts&...>
  [[nodiscard]] auto then(std::string_view name, Derive&& derive) && {
    using DerivationResult = std::invoke_result_t<Derive&, const value_type&, const Facts&...>;
    using Derived = std::remove_cvref_t<DerivationResult>;
    static_assert(!std::is_reference_v<DerivationResult>,
                  "suppose.then: return a Pluss value, not a reference");
    static_assert(supposition_value<Derived>,
                  "suppose.then: a derivation must produce a Pluss value");
    if (name.empty()) throw std::invalid_argument("suppose.then: step name must not be empty");
    std::optional<Derived> next;
    if (status_ == SuppositionStatus::verified) {
      next.emplace(std::apply([&](const auto&... prior) {
        return std::invoke(std::forward<Derive>(derive), value_, *prior...);
      }, facts_));
      steps_.push_back({std::string(name), SuppositionStepKind::derivation,
                        SuppositionStepStatus::computed});
    } else {
      steps_.push_back({std::string(name), SuppositionStepKind::derivation,
                        SuppositionStepStatus::skipped});
    }
    auto all_facts = std::tuple_cat(std::move(facts_), std::make_tuple(std::move(next)));
    return Supposition<T, Facts..., Derived>(std::forward<T>(value_), status_,
                                             std::move(steps_), std::move(all_facts));
  }

  template <class Predicate>
    requires std::predicate<Predicate&, const value_type&, const Facts&...>
  Supposition&& check(std::string_view name, Predicate&& predicate) && {
    if (name.empty()) throw std::invalid_argument("suppose.check: step name must not be empty");
    if (status_ == SuppositionStatus::verified) {
      const bool passed = std::apply([&](const auto&... prior) {
        return static_cast<bool>(std::invoke(std::forward<Predicate>(predicate), value_, *prior...));
      }, facts_);
      steps_.push_back({std::string(name), SuppositionStepKind::check,
                        passed ? SuppositionStepStatus::passed : SuppositionStepStatus::failed});
      if (!passed) status_ = SuppositionStatus::derivation_failed;
    } else {
      steps_.push_back({std::string(name), SuppositionStepKind::check,
                        SuppositionStepStatus::skipped});
    }
    return std::move(*this);
  }

  template <class Derive, class Verify>
    requires std::invocable<Derive&, const value_type&, const Facts&...>
  [[nodiscard]] auto then(std::string_view name, Derive&& derive, Verify&& verify) && {
    using DerivationResult = std::invoke_result_t<Derive&, const value_type&, const Facts&...>;
    using Derived = std::remove_cvref_t<DerivationResult>;
    static_assert(!std::is_reference_v<DerivationResult>,
                  "suppose.then: return a Pluss value, not a reference");
    static_assert(supposition_value<Derived>,
                  "suppose.then: a derivation must produce a Pluss value");
    static_assert(std::predicate<Verify&, const value_type&, const Facts&..., const Derived&>,
                  "suppose.then: verification must accept the premise, prior facts, and new fact");
    if (name.empty()) throw std::invalid_argument("suppose.then: step name must not be empty");
    std::optional<Derived> next;
    if (status_ == SuppositionStatus::verified) {
      next.emplace(std::apply([&](const auto&... prior) {
        return std::invoke(std::forward<Derive>(derive), value_, *prior...);
      }, facts_));
      const bool passed = std::apply([&](const auto&... prior) {
        return static_cast<bool>(std::invoke(std::forward<Verify>(verify),
                                             value_, *prior..., *next));
      }, facts_);
      steps_.push_back({std::string(name), SuppositionStepKind::derivation,
                        passed ? SuppositionStepStatus::passed : SuppositionStepStatus::failed});
      if (!passed) status_ = SuppositionStatus::derivation_failed;
    } else {
      steps_.push_back({std::string(name), SuppositionStepKind::derivation,
                        SuppositionStepStatus::skipped});
    }
    auto all_facts = std::tuple_cat(std::move(facts_), std::make_tuple(std::move(next)));
    return Supposition<T, Facts..., Derived>(std::forward<T>(value_), status_,
                                             std::move(steps_), std::move(all_facts));
  }

  template <class Predicate>
    requires std::predicate<Predicate&, const value_type&, const Facts&...>
  [[nodiscard]] SuppositionResult<Facts...> conclude(std::string_view name,
                                                       Predicate&& check) && {
    if (name.empty()) throw std::invalid_argument("suppose.conclude: conclusion name must not be empty");
    if (status_ != SuppositionStatus::verified) {
      steps_.push_back({std::string(name), SuppositionStepKind::conclusion,
                        SuppositionStepStatus::skipped});
    } else {
      const bool passed = std::apply([&](const auto&... prior) {
        return static_cast<bool>(std::invoke(std::forward<Predicate>(check), value_, *prior...));
      }, facts_);
      steps_.push_back({std::string(name), SuppositionStepKind::conclusion,
                        passed ? SuppositionStepStatus::passed : SuppositionStepStatus::failed});
      if (!passed) status_ = SuppositionStatus::conclusion_failed;
    }
    return {status_, std::move(steps_), std::move(facts_)};
  }

 private:
  template <class, class...> friend class Supposition;

  Supposition(T value, SuppositionStatus status, std::vector<SuppositionStep> steps,
              std::tuple<std::optional<Facts>...> facts)
      : value_(std::forward<T>(value)), status_(status), steps_(std::move(steps)),
        facts_(std::move(facts)) {}

  T value_;
  SuppositionStatus status_;
  std::vector<SuppositionStep> steps_;
  std::tuple<std::optional<Facts>...> facts_;
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
