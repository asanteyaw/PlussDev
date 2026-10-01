#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
#include <limits>
#include <sstream>
#include <vector>

TEST_CASE("Duration unit factories preserve exact values and shape",
          "[codeh][duration][factory]") {
  constexpr std::int64_t exact = 9'007'199'254'740'993LL;
  REQUIRE(pluss::duration::nanos(exact).host() ==
          std::vector<std::int64_t>{exact});
  REQUIRE(pluss::duration::microseconds({1, -2}).host() ==
          std::vector<std::int64_t>{1'000, -2'000});
  REQUIRE(pluss::duration::milliseconds({{1, 2}, {3, 4}}).sizes() ==
          std::vector<std::int64_t>{2, 2});
  REQUIRE(pluss::duration::seconds({1, 2, 3, 4}, {2, 2}).host() ==
          std::vector<std::int64_t>{1'000'000'000LL, 2'000'000'000LL,
                                    3'000'000'000LL, 4'000'000'000LL});
  REQUIRE(pluss::duration::minutes(1).host()[0] == 60'000'000'000LL);
  REQUIRE(pluss::duration::hours(1).host()[0] == 3'600'000'000'000LL);
  REQUIRE(pluss::duration::days(1).host()[0] == 86'400'000'000'000LL);
  REQUIRE(pluss::duration::weeks(1).host()[0] == 604'800'000'000'000LL);
  REQUIRE_THROWS(pluss::duration::weeks(
      (std::numeric_limits<std::int64_t>::max)()));
}

TEST_CASE("Duration make and root shortcut have identical contracts",
          "[codeh][duration][factory]") {
  const auto canonical = pluss::duration::make(
      {1, 2, 3, 4}, {2, 2}, pluss::duration::Unit::Minutes);
  const auto shortcut = pluss::make_duration(
      {1, 2, 3, 4}, {2, 2}, pluss::duration::Unit::Minutes);
  REQUIRE(canonical.sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(canonical.host() == shortcut.host());
  REQUIRE_THROWS(pluss::duration::make(
      1, static_cast<pluss::duration::Unit>(999)));
}

TEST_CASE("Duration range handles direction endpoints and integer boundaries",
          "[codeh][duration][factory]") {
  REQUIRE(pluss::duration::range(pluss::duration::seconds(0),
                                 pluss::duration::seconds(5),
                                 pluss::duration::seconds(2)).host() ==
          std::vector<std::int64_t>{0, 2'000'000'000LL, 4'000'000'000LL});
  REQUIRE(pluss::duration::range(pluss::duration::seconds(5),
                                 pluss::duration::seconds(0),
                                 pluss::duration::seconds(-2)).host() ==
          std::vector<std::int64_t>{5'000'000'000LL, 3'000'000'000LL,
                                    1'000'000'000LL});
  REQUIRE(pluss::duration::range(pluss::duration::seconds(5),
                                 pluss::duration::seconds(0),
                                 pluss::duration::seconds(2)).numel() == 0);
  const auto hi = (std::numeric_limits<std::int64_t>::max)();
  REQUIRE(pluss::duration::range(pluss::duration::nanos(hi - 1),
                                 pluss::duration::nanos(hi),
                                 pluss::duration::nanos(1)).host() ==
          std::vector<std::int64_t>{hi - 1, hi});
  REQUIRE_THROWS(pluss::duration::range(pluss::duration::seconds(0),
                                        pluss::duration::seconds(1),
                                        pluss::duration::seconds(0)));
}

TEST_CASE("Duration shape factories validate and preserve geometry",
          "[codeh][duration][factory]") {
  REQUIRE(pluss::duration::zeros({2, 3}).host() ==
          std::vector<std::int64_t>(6, 0));
  const auto like = pluss::duration::seconds({1, 2, 3, 4}, {2, 2});
  REQUIRE(pluss::duration::zeros_like(like).sizes() == like.sizes());
  REQUIRE(pluss::duration::full({2, 2}, pluss::duration::seconds(-3)).host() ==
          std::vector<std::int64_t>(4, -3'000'000'000LL));
  REQUIRE(pluss::duration::full_like(like, pluss::duration::nanos(7)).host() ==
          std::vector<std::int64_t>(4, 7));
  REQUIRE_THROWS(pluss::duration::zeros({2, -1}));
  REQUIRE_THROWS(pluss::duration::full({2, 2}, pluss::duration::seconds({1, 2})));
}

TEST_CASE("Duration random is seeded exact and half open",
          "[codeh][duration][factory]") {
  const auto lo = pluss::duration::nanos(10);
  const auto hi = pluss::duration::nanos(20);
  const auto a = pluss::duration::random({2, 3}, lo, hi, 42);
  const auto b = pluss::duration::random({2, 3}, lo, hi, 42);
  REQUIRE(a.host() == b.host());
  REQUIRE(a.sizes() == std::vector<std::int64_t>{2, 3});
  for (const auto value : a.host()) {
    REQUIRE(value >= 10);
    REQUIRE(value < 20);
  }
  REQUIRE_THROWS(pluss::duration::random({2}, hi, lo, 42));
  REQUIRE_THROWS(pluss::duration::random(
      {2}, pluss::duration::nanos(pluss::Duration::kNaT), hi, 42));
}

TEST_CASE("Duration print overloads agree with stream insertion",
          "[codeh][duration][factory]") {
  const auto value = pluss::duration::nanos({0, -1});
  std::ostringstream via_factory;
  pluss::duration::print(via_factory, value);
  std::ostringstream via_operator;
  via_operator << value;
  REQUIRE(via_factory.str() == pluss::duration::print(value));
  REQUIRE(via_factory.str() == via_operator.str());
}
