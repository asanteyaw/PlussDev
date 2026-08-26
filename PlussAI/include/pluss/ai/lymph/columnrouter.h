#pragma once

#include "pluss/ai/lymph/common.h"

#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

// Route, relocated out of common.h into its own per-concern file
// (LYMPH_REDESIGN_PLAN.md phase 1: pure move, no behavior change). The one
// real change: construction moves off static methods on the class
// (old ColumnRouter::names(...), ::where(...), ::remaining_of(...)) into the
// prep::route family surface, per the construction-vs-method
// rule (LYMPH_REDESIGN_PLAN.md 2.0 rule 2) -- `Type::factory()` syntax
// forces the left of `::` to match the class name exactly, so it can never
// be lowercased; the class itself now keeps no construction of its own,
// only .resolve(...) and .with_remainder(...).

namespace pluss::ai::lymph::prep {

// Forward-declared here (Route is still incomplete at this point,
// which is fine for a function declaration -- only a definition or a call
// needs the complete type) so ColumnRouter's own friend declarations below
// can name them.
namespace route {

class Route;

[[nodiscard]] Route names(std::vector<std::string> names);
[[nodiscard]] Route where(std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate);
[[nodiscard]] Route remaining_of(std::vector<std::string> names);
[[nodiscard]] Route remaining_of(Route other);

} // namespace route

using Route = route::Route;

// A real object unifying route()/route_where()/route_remaining() + a
// remainder policy into one reusable, declarative column selection, closing
// the "several free functions a caller has to stitch together by hand every
// time" gap: build a Route once per prep stage (route::
// names(...), where(...), or remaining_of(...)), then call
// .resolve(frame, ordered_names) as many times as needed (a fresh DataFrame
// each fold/call) instead of re-deriving a feature_route inline at every
// call site. resolve() always walks the explicit `ordered_names` list
// rather than frame.feataxis().series().features(), so a Note-typed column
// is visible to a where(...) router the same way the two-argument
// route_where(frame, ordered_names, predicate) overload in common.h already
// fixed -- this class has no independent Note-blind code path.
class route::Route final {
 public:
  Route& with_remainder(RouteRemainderPolicy policy) noexcept {
    remainder = policy;
    return *this;
  }

  // How a caller applies this router's selection to a prep stage's own
  // remainder handling once feature_route is resolved (e.g.
  // Model::set_stage_remainder_); carried alongside the selection itself
  // rather than threaded through a parallel argument at every call site.
  RouteRemainderPolicy remainder = RouteRemainderPolicy::PassThrough;

  [[nodiscard]] std::vector<std::int64_t> resolve(const pluss::table::DataFrame& frame,
                                                   const std::vector<std::string>& ordered_names) const {
    switch (kind_) {
      case Kind::Names: {
        const auto views = detail::as_string_views_(names_);
        return ::pluss::ai::lymph::route(ordered_names, pluss::msharp::RangeView::Of<std::string_view>(views));
      }
      case Kind::Where:
        return ::pluss::ai::lymph::route_where(frame, ordered_names, predicate_);
      case Kind::RemainingOfNames: {
        const auto views = detail::as_string_views_(names_);
        return ::pluss::ai::lymph::route_remaining(ordered_names, pluss::msharp::RangeView::Of<std::string_view>(views));
      }
      case Kind::RemainingOfRouter: {
        const auto claimed = remaining_of_->resolve(frame, ordered_names);
        const std::unordered_set<std::int64_t> claimed_set(claimed.begin(), claimed.end());
        // Ranges filter over the index space rather than a hand-rolled
        // push_back loop. Not a perf-motivated change -- column counts are
        // always small (tens, never millions), and there is no Series/
        // tensor data anywhere in Route to vectorize onto a device;
        // this is pure host-side name/index bookkeeping. The filter reads
        // as "every index not already claimed," which is what it is.
        // Not `const`: libc++'s filter_view::begin()/end() aren't const
        // member functions (the predicate storage isn't guaranteed
        // const-callable), so a const view can't be iterated.
        auto kept_view = std::views::iota(std::int64_t{0}, static_cast<std::int64_t>(ordered_names.size())) |
                         std::views::filter([&claimed_set](std::int64_t i) { return !claimed_set.contains(i); });
        return std::vector<std::int64_t>(kept_view.begin(), kept_view.end());
      }
    }
    ::pluss::throw_error("lymph::prep::Route::resolve: unreachable selection kind.");
  }

  // Convenience default: resolve against the frame's own Series-only column
  // order -- correct whenever the caller isn't using
  // DataNotePolicy::Factorize. Pass the DataFrameBatchOptions::feature_names
  // list explicitly to the two-argument overload above once Note columns
  // are in play, the same rule route()/route_where()'s own two forms
  // already follow.
  [[nodiscard]] std::vector<std::int64_t> resolve(const pluss::table::DataFrame& frame) const {
    return resolve(frame, frame.feataxis().series().features());
  }

private:
  enum class Kind { Names, Where, RemainingOfNames, RemainingOfRouter };

  Route() = default;

  Kind kind_ = Kind::Names;
  std::vector<std::string> names_{};
  std::function<bool(std::string_view, std::string_view)> predicate_{};
  std::shared_ptr<Route> remaining_of_{};

  friend Route names(std::vector<std::string> names);
  friend Route where(std::function<bool(std::string_view, std::string_view)> predicate);
  friend Route remaining_of(std::vector<std::string> names);
  friend Route remaining_of(Route other);
};

namespace route {

[[nodiscard]] inline Route names(std::vector<std::string> names) {
  Route out{};
  out.kind_ = Route::Kind::Names;
  out.names_ = std::move(names);
  return out;
}

[[nodiscard]] inline Route where(
    std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate) {
  Route out{};
  out.kind_ = Route::Kind::Where;
  out.predicate_ = std::move(predicate);
  return out;
}

// Everything not in `names` -- the Route-native shape of
// route_remaining().
[[nodiscard]] inline Route remaining_of(std::vector<std::string> names) {
  Route out{};
  out.kind_ = Route::Kind::RemainingOfNames;
  out.names_ = std::move(names);
  return out;
}

// Everything not already claimed by `other` -- lets a remainder stay
// declaratively tied to whatever selection strategy `other` uses (names or
// where) instead of duplicating its logic as a separately-maintained name
// list that can drift out of sync with it.
[[nodiscard]] inline Route remaining_of(Route other) {
  Route out{};
  out.kind_ = Route::Kind::RemainingOfRouter;
  out.remaining_of_ = std::make_shared<Route>(std::move(other));
  return out;
}

} // namespace route

} // namespace pluss::ai::lymph::prep
