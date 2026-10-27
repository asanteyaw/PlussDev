#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<std::string> text_values(const pluss::Note& note) {
  std::vector<std::string> out;
  for (const auto value : note.vector()) out.emplace_back(value);
  return out;
}

} // namespace

TEST_CASE("Duration rounding uses floor ceil and half-even contracts",
          "[codeh][duration][rounding]") {
  constexpr std::int64_t second = 1'000'000'000LL;
  const auto values = pluss::duration::nanos({
      -1'600'000'000LL, -1'500'000'000LL, -500'000'000LL,
       500'000'000LL,  1'500'000'000LL, 2'500'000'000LL,
       1'400'000'000LL, pluss::Duration::kNaT,
  });

  REQUIRE(values.floor("s").host() == std::vector<std::int64_t>{
      -2 * second, -2 * second, -second, 0, second, 2 * second,
      second, pluss::Duration::kNaT});
  REQUIRE(values.ceil("s").host() == std::vector<std::int64_t>{
      -second, -second, 0, second, 2 * second, 3 * second,
      2 * second, pluss::Duration::kNaT});
  REQUIRE(values.round("s").host() == std::vector<std::int64_t>{
      -2 * second, -2 * second, 0, 0, 2 * second, 2 * second,
      second, pluss::Duration::kNaT});

  const auto exact = pluss::duration::nanos(1'234'567'891LL);
  REQUIRE(exact.floor("ns").host()[0] == 1'234'567'891LL);
  REQUIRE(exact.floor("us").host()[0] == 1'234'567'000LL);
  REQUIRE(exact.floor("ms").host()[0] == 1'234'000'000LL);
  REQUIRE(exact.floor("min").host()[0] == 0);
  REQUIRE(exact.floor("h").host()[0] == 0);
  REQUIRE(exact.floor("D").host()[0] == 0);
  REQUIRE(exact.floor("W").host()[0] == 0);

  REQUIRE_THROWS(exact.floor("second"));
  REQUIRE_THROWS(pluss::duration::nanos((std::numeric_limits<std::int64_t>::max)())
                     .ceil("D"));
}

TEST_CASE("Duration extrema reduce globally with explicit NaT policy",
          "[codeh][duration][extrema]") {
  const auto values = pluss::duration::nanos(
      {5, pluss::Duration::kNaT, -2, 9}, {2, 2});
  REQUIRE(values.min().host() == std::vector<std::int64_t>{-2});
  REQUIRE(values.max().host() == std::vector<std::int64_t>{9});
  REQUIRE(values.min().sizes().empty());
  REQUIRE(values.max().sizes().empty());
  REQUIRE(values.min(false).host() ==
          std::vector<std::int64_t>{pluss::Duration::kNaT});
  REQUIRE(values.max(false).host() ==
          std::vector<std::int64_t>{pluss::Duration::kNaT});

  const auto all_nat = pluss::duration::nanos(
      {pluss::Duration::kNaT, pluss::Duration::kNaT});
  const auto empty = pluss::duration::nanos(std::vector<std::int64_t>{});
  REQUIRE(all_nat.min().host()[0] == pluss::Duration::kNaT);
  REQUIRE(all_nat.max().host()[0] == pluss::Duration::kNaT);
  REQUIRE(empty.min().host()[0] == pluss::Duration::kNaT);
  REQUIRE(empty.max().host()[0] == pluss::Duration::kNaT);
}

TEST_CASE("Duration ordering is stable and places NaT last",
          "[codeh][duration][ordering]") {
  const auto values = pluss::duration::nanos(
      {3, pluss::Duration::kNaT, 1, 3, pluss::Duration::kNaT, 2});
  REQUIRE(values.sort().host() == std::vector<std::int64_t>{
      1, 2, 3, 3, pluss::Duration::kNaT, pluss::Duration::kNaT});
  REQUIRE(values.argsort().host<std::int64_t>() ==
          std::vector<std::int64_t>{2, 5, 0, 3, 1, 4});
  REQUIRE(values.unique().host() == std::vector<std::int64_t>{
      3, pluss::Duration::kNaT, 1, 2});

  const auto empty = pluss::duration::nanos(std::vector<std::int64_t>{});
  REQUIRE(empty.sort().numel() == 0);
  REQUIRE(empty.argsort().numel() == 0);
  REQUIRE(empty.unique().numel() == 0);

  const auto matrix = pluss::duration::nanos({1, 2, 3, 4}, {2, 2});
  REQUIRE_THROWS(matrix.sort());
  REQUIRE_THROWS(matrix.argsort());
  REQUIRE_THROWS(matrix.unique());
}

TEST_CASE("Duration Note formatting is normalized and ISO is distinct",
          "[codeh][duration][formatting]") {
  const auto values = pluss::duration::nanos({
      93'784'005'006'007LL, -1, 0, pluss::Duration::kNaT});
  REQUIRE(text_values(values.to_note()) == std::vector<std::string>{
      "1 days 02:03:04.005006007",
      "-1 days 23:59:59.999999999",
      "0 days 00:00:00.000000000",
      "NaT"});
  REQUIRE(text_values(values.to_note_iso()) == std::vector<std::string>{
      "P1DT2H3M4.005006007S",
      "-PT0.000000001S",
      "PT0S",
      "NaT"});

  const auto matrix = pluss::duration::nanos({1, 2, 3, 4}, {2, 2});
  REQUIRE(matrix.to_note().sizes() == std::vector<std::int64_t>{2, 2});
  REQUIRE(matrix.to_note_iso().sizes() == std::vector<std::int64_t>{2, 2});

  const auto negative_tick = pluss::duration::nanos(-1);
  REQUIRE(pluss::duration::print(negative_tick).find(
              "-1 days 23:59:59.999999999") != std::string::npos);
  std::ostringstream stream;
  stream << negative_tick;
  REQUIRE(stream.str() == pluss::duration::print(negative_tick));
}

TEST_CASE("Duration arithmetic rejects nanosecond overflow",
          "[codeh][duration][operators]") {
  const auto hi = pluss::duration::nanos((std::numeric_limits<std::int64_t>::max)());
  const auto lo = pluss::duration::nanos((std::numeric_limits<std::int64_t>::min)() + 1);
  const auto one = pluss::duration::nanos(1);

  REQUIRE_THROWS(hi + one);
  REQUIRE_THROWS(lo - one);
  REQUIRE_THROWS(hi * 2);
}
