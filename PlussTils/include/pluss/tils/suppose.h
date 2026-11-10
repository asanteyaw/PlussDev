#pragma once

// `suppose("premise", pluss_value, predicate)` checks a named premise,
// typed derivations, and a conclusion for a concrete Pluss value. Value pipelines
// use `flow(value)`; `SupposeBox` remains as an alias for older source code.

#include <concepts>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <memory>
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

template <class T>
using SupposeBox = FlowBox<T>;

enum class SuppositionStatus {
  completed,
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
  [[nodiscard]] bool completed() const noexcept {
    return status_ == SuppositionStatus::completed || verified();
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

  [[nodiscard]] SuppositionResult<Facts...> finish() && {
    if (status_ == SuppositionStatus::verified) status_ = SuppositionStatus::completed;
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

enum class BlueprintKind { given, step, claim, goal };

struct BlueprintNode {
  std::string name;
  BlueprintKind kind;
  std::string reason;
  std::vector<std::size_t> dependencies;
  bool done;
};

// A blueprint is a dependency graph, not a record of executed or proved claims.
template <supposition_value T>
class SupposeBlueprint {
 public:
  using id_type = std::size_t;

  explicit SupposeBlueprint(std::string name) : name_(std::move(name)) {
    if (name_.empty()) throw std::invalid_argument("suppose.blueprint: name must not be empty");
  }

  SupposeBlueprint& given(std::string_view name) {
    add(name, BlueprintKind::given, {}, {}, true, false);
    return *this;
  }
  SupposeBlueprint& step(std::string_view name) {
    add(name, BlueprintKind::step, {}, {}, false, true);
    return *this;
  }
  SupposeBlueprint& claim(std::string_view name, std::string_view reason) {
    if (reason.empty()) throw std::invalid_argument("suppose.outline: claim needs a reason");
    add(name, BlueprintKind::claim, reason, {}, false, true);
    return *this;
  }
  SupposeBlueprint& goal(std::string_view name) {
    add(name, BlueprintKind::goal, {}, {}, false, true);
    return *this;
  }
  // Explicit dependencies allow branches; the one-argument form follows the previous node.
  id_type step_after(std::string_view name, std::initializer_list<id_type> dependencies) {
    return add(name, BlueprintKind::step, {}, dependencies, false, false);
  }
  id_type claim_after(std::string_view name, std::string_view reason,
                      std::initializer_list<id_type> dependencies) {
    if (reason.empty()) throw std::invalid_argument("suppose.outline: claim needs a reason");
    return add(name, BlueprintKind::claim, reason, dependencies, false, false);
  }
  id_type goal_after(std::string_view name, std::initializer_list<id_type> dependencies) {
    return add(name, BlueprintKind::goal, {}, dependencies, false, false);
  }
  id_type last_id() const {
    if (nodes_.empty()) throw std::logic_error("suppose.blueprint: no steps");
    return nodes_.size() - 1;
  }
  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  [[nodiscard]] const std::vector<BlueprintNode>& nodes() const noexcept { return nodes_; }
  [[nodiscard]] std::vector<id_type> ready() const {
    std::vector<id_type> result;
    for (id_type id = 0; id < nodes_.size(); ++id) {
      if (nodes_[id].done) continue;
      const auto& dependencies = nodes_[id].dependencies;
      if (std::all_of(dependencies.begin(), dependencies.end(),
                      [&](id_type parent) { return nodes_[parent].done; })) result.push_back(id);
    }
    return result;
  }
  void mark_done(id_type id) {
    if (id >= nodes_.size()) throw std::out_of_range("suppose.blueprint: unknown step");
    if (nodes_[id].done) return;
    const auto available = ready();
    if (std::find(available.begin(), available.end(), id) == available.end())
      throw std::logic_error("suppose.blueprint: dependencies are not complete");
    nodes_[id].done = true;
  }

 private:
  id_type add(std::string_view name, BlueprintKind kind, std::string_view reason,
              std::initializer_list<id_type> dependencies, bool done, bool follow_previous) {
    if (name.empty()) throw std::invalid_argument("suppose.blueprint: step name must not be empty");
    std::vector<id_type> parents(dependencies);
    if (parents.empty() && follow_previous && !nodes_.empty())
      parents.push_back(nodes_.size() - 1);
    for (id_type parent : parents)
      if (parent >= nodes_.size()) throw std::out_of_range("suppose.blueprint: unknown dependency");
    std::sort(parents.begin(), parents.end());
    parents.erase(std::unique(parents.begin(), parents.end()), parents.end());
    nodes_.push_back({std::string(name), kind, std::string(reason), std::move(parents), done});
    return nodes_.size() - 1;
  }

  std::string name_;
  std::vector<BlueprintNode> nodes_;
};

// The proof kernel checks propositional inference. Atomic statements are opaque;
// it does not invent numerical facts or assume a runtime observation is universal.
template <supposition_value T>
class Proposition {
 public:
  enum class Kind { atom, conjunction, implication };

  static Proposition atom(std::string name) {
    if (name.empty()) throw std::invalid_argument("suppose.atom: name must not be empty");
    return Proposition(std::make_shared<Node>(Node{Kind::atom, std::move(name), {}, {}}));
  }
  [[nodiscard]] Proposition and_(const Proposition& other) const {
    return Proposition(std::make_shared<Node>(Node{Kind::conjunction, {}, node_, other.node_}));
  }
  [[nodiscard]] Proposition implies(const Proposition& other) const {
    return Proposition(std::make_shared<Node>(Node{Kind::implication, {}, node_, other.node_}));
  }
  [[nodiscard]] Kind kind() const noexcept { return node_->kind; }
  [[nodiscard]] Proposition left() const {
    if (node_->kind == Kind::atom) throw std::logic_error("suppose.proof: atom has no left side");
    return Proposition(node_->left);
  }
  [[nodiscard]] Proposition right() const {
    if (node_->kind == Kind::atom) throw std::logic_error("suppose.proof: atom has no right side");
    return Proposition(node_->right);
  }
  [[nodiscard]] std::string text() const {
    if (kind() == Kind::atom) return node_->name;
    return "(" + left().text() + (kind() == Kind::conjunction ? " and " : " implies ")
           + right().text() + ")";
  }
  [[nodiscard]] bool operator==(const Proposition& other) const noexcept {
    return equal(node_, other.node_);
  }

 private:
  struct Node {
    Kind kind;
    std::string name;
    std::shared_ptr<const Node> left;
    std::shared_ptr<const Node> right;
  };
  explicit Proposition(std::shared_ptr<const Node> node) : node_(std::move(node)) {}
  static bool equal(const std::shared_ptr<const Node>& a,
                    const std::shared_ptr<const Node>& b) noexcept {
    if (a == b) return true;
    if (!a || !b || a->kind != b->kind) return false;
    if (a->kind == Kind::atom) return a->name == b->name;
    return equal(a->left, b->left) && equal(a->right, b->right);
  }
  std::shared_ptr<const Node> node_;
};

template <supposition_value T> class SupposeProof;

class ProofCertificate {
 public:
  [[nodiscard]] const std::string& conclusion() const noexcept { return conclusion_; }
  [[nodiscard]] const std::vector<std::string>& assumptions() const noexcept {
    return assumptions_;
  }
  [[nodiscard]] bool unconditional() const noexcept { return assumptions_.empty(); }

 private:
  template <supposition_value T> friend class SupposeProof;
  ProofCertificate(std::string conclusion, std::vector<std::string> assumptions)
      : conclusion_(std::move(conclusion)), assumptions_(std::move(assumptions)) {}
  std::string conclusion_;
  std::vector<std::string> assumptions_;
};

template <supposition_value T>
class SupposeProof {
 public:
  using Formula = Proposition<T>;
  using id_type = std::size_t;

  id_type assume(Formula statement) {
    const id_type id = lines_.size();
    lines_.push_back({std::move(statement), {id}, true});
    return id;
  }
  id_type and_intro(id_type left, id_type right) {
    return add(line(left).formula.and_(line(right).formula), merge(line(left), line(right)));
  }
  id_type and_left(id_type pair) {
    const auto& source = line(pair);
    if (source.formula.kind() != Formula::Kind::conjunction)
      throw std::logic_error("suppose.proof: and_left requires a conjunction");
    return add(source.formula.left(), source.assumptions);
  }
  id_type and_right(id_type pair) {
    const auto& source = line(pair);
    if (source.formula.kind() != Formula::Kind::conjunction)
      throw std::logic_error("suppose.proof: and_right requires a conjunction");
    return add(source.formula.right(), source.assumptions);
  }
  id_type modus_ponens(id_type antecedent, id_type implication) {
    const auto& premise = line(antecedent);
    const auto& rule = line(implication);
    if (rule.formula.kind() != Formula::Kind::implication ||
        !(rule.formula.left() == premise.formula))
      throw std::logic_error("suppose.proof: implication does not match antecedent");
    return add(rule.formula.right(), merge(premise, rule));
  }
  id_type discharge(id_type assumption, id_type conclusion) {
    const auto& given = line(assumption);
    const auto& proved = line(conclusion);
    if (!given.is_assumption) throw std::logic_error("suppose.proof: line is not an assumption");
    auto remaining = proved.assumptions;
    remaining.erase(std::remove(remaining.begin(), remaining.end(), assumption), remaining.end());
    return add(given.formula.implies(proved.formula), std::move(remaining));
  }
  [[nodiscard]] ProofCertificate conclude(id_type line_id, const Formula& expected) const {
    const auto& proved = line(line_id);
    if (!(proved.formula == expected))
      throw std::logic_error("suppose.proof: conclusion does not match proved line");
    std::vector<std::string> assumptions;
    for (id_type id : proved.assumptions) assumptions.push_back(line(id).formula.text());
    return ProofCertificate(expected.text(), std::move(assumptions));
  }

 private:
  struct Line {
    Formula formula;
    std::vector<id_type> assumptions;
    bool is_assumption;
  };
  const Line& line(id_type id) const {
    if (id >= lines_.size()) throw std::out_of_range("suppose.proof: unknown line");
    return lines_[id];
  }
  id_type add(Formula formula, std::vector<id_type> assumptions) {
    const id_type id = lines_.size();
    lines_.push_back({std::move(formula), std::move(assumptions), false});
    return id;
  }
  static std::vector<id_type> merge(const Line& a, const Line& b) {
    std::vector<id_type> result = a.assumptions;
    result.insert(result.end(), b.assumptions.begin(), b.assumptions.end());
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
  }
  std::vector<Line> lines_;
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
  template <supposition_value T>
  [[nodiscard]] auto blueprint(std::string name) const {
    return SupposeBlueprint<std::remove_cvref_t<T>>(std::move(name));
  }

  template <supposition_value T>
  [[nodiscard]] auto outline(std::string name) const {
    return SupposeBlueprint<std::remove_cvref_t<T>>(std::move(name));
  }

  template <supposition_value T>
  [[nodiscard]] auto atom(std::string name) const {
    return Proposition<std::remove_cvref_t<T>>::atom(std::move(name));
  }

  template <supposition_value T>
  [[nodiscard]] auto proof() const {
    return SupposeProof<std::remove_cvref_t<T>>{};
  }

  template <supposition_value T>
  [[nodiscard]] auto operator()(T&& value) const {
    return (*this)("given", std::forward<T>(value), [](const auto&) { return true; });
  }

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
