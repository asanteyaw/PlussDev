#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Thread;
using namespace pluss::ai::lymph;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

bool contains(const std::vector<std::int64_t>& indices, std::int64_t value) {
  for (auto v : indices) {
    if (v == value) return true;
  }
  return false;
}

bool run_column_router_checks(std::ostream* err) {
  // A frame with two Series columns and one Note column, matching the
  // ordered_names a DataNotePolicy::Factorize batch would actually use
  // ("x", "z" as Series, "category" as the factorized Note column, in that
  // order) -- the exact shape route_where(frame, predicate) was blind to.
  const Series x = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, f64());
  const Series z = pluss::make_series(std::vector<double>{4.0, 5.0, 6.0}, {3}, f64());
  const DataFrame frame({
      {"x", Thread(x)},
      {"category", Thread(pluss::make_note({"red", "green", "blue"}))},
      {"z", Thread(z)},
  });
  const std::vector<std::string> ordered_names = {"x", "category", "z"};

  // 1) route_where(frame, predicate) (Series-only, pre-existing) can never
  // select "category" -- it isn't even in its iteration order.
  const auto series_only = route_where(frame, [](std::string_view, std::string_view) { return true; });
  if (series_only.size() != 2) {
    if (err) *err << "route_where(frame, predicate) should only ever see the 2 Series columns\n";
    return false;
  }

  // 2) route_where(frame, ordered_names, predicate) -- the Note-aware
  // sibling -- CAN select "category" by name, at its correct position (1)
  // in ordered_names.
  const auto note_aware = route_where(frame, ordered_names,
                                      [](std::string_view name, std::string_view) { return name == "category"; });
  if (note_aware.size() != 1 || note_aware[0] != 1) {
    if (err) *err << "route_where(frame, ordered_names, predicate) should select \"category\" at index 1\n";
    return false;
  }

  // 3) prep::route::names/remaining_of round-trip against ordered_names.
  const auto x_only = prep::route::names({"x"}).resolve(frame, ordered_names);
  if (x_only != std::vector<std::int64_t>{0}) {
    if (err) *err << "prep::route::names({\"x\"}) should resolve to index 0\n";
    return false;
  }
  const auto remainder_of_x = prep::route::remaining_of(std::vector<std::string>{"x"}).resolve(frame, ordered_names);
  if (remainder_of_x.size() != 2 || !contains(remainder_of_x, 1) || !contains(remainder_of_x, 2)) {
    if (err) *err << "prep::route::remaining_of({\"x\"}) should resolve to {category, z} = {1, 2}\n";
    return false;
  }

  // 4) prep::route::where is Note-aware (unlike the bare frame-only
  // route_where), and prep::route::remaining_of(router) stays declaratively
  // tied to it instead of a separately hand-maintained name list.
  const auto category_router = prep::route::where(
      [](std::string_view name, std::string_view) { return name == "category"; });
  const auto category_only = category_router.resolve(frame, ordered_names);
  if (category_only != std::vector<std::int64_t>{1}) {
    if (err) *err << "prep::route::where(...) should be able to select the Note column \"category\"\n";
    return false;
  }
  const auto remainder_of_category = prep::route::remaining_of(category_router).resolve(frame, ordered_names);
  if (remainder_of_category.size() != 2 || !contains(remainder_of_category, 0) || !contains(remainder_of_category, 2)) {
    if (err) *err << "prep::route::remaining_of(where-router) should resolve to {x, z} = {0, 2}\n";
    return false;
  }

  // 5) remainder policy travels with the router as plain data.
  prep::Route with_drop = prep::route::names({"x"}).with_remainder(RouteRemainderPolicy::Drop);
  if (with_drop.remainder != RouteRemainderPolicy::Drop) {
    if (err) *err << "prep::Route::with_remainder should set .remainder\n";
    return false;
  }

  // 6) Convenience one-argument resolve() matches the Series-only frame
  // order for a Series-only selection (no Note columns involved).
  const DataFrame series_only_frame({{"x", Thread(x)}, {"z", Thread(z)}});
  const auto default_resolved = prep::route::names({"x"}).resolve(series_only_frame);
  if (default_resolved != std::vector<std::int64_t>{0}) {
    if (err) *err << "prep::Route::resolve(frame) convenience overload should match the Series-only order\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph prep::Route", "[lymph][prep][route]") {
  if (!run_column_router_checks(&std::cerr)) {
    FAIL("lymph column_router checks failed");
  }
}
#else
int main() {
  if (!run_column_router_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph column_router checks passed\n";
  return 0;
}
#endif
