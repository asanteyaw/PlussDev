#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace {

void require_nan(double value) {
  REQUIRE(std::isnan(value));
}

auto make_index(std::initializer_list<const char*> values,
                bool require_sorted = true) {
  return pluss::timeline::make(values).index(require_sorted);
}

auto make_index(const char* value) {
  return pluss::timeline::make({value}).index();
}

auto make_index(std::vector<std::int64_t> values) {
  return pluss::timeline::epoch(std::move(values)).index();
}

} // namespace

TEST_CASE("TimeIndex decreasing monotonicity accepts ties and trivial axes",
          "[timeindex][monotonic]") {
  const auto descending = make_index({
      "2025-01-03T00:00:00Z",
      "2025-01-02T00:00:00Z",
      "2025-01-02T00:00:00Z",
      "2025-01-01T00:00:00Z",
  }, false);
  const auto ascending = make_index({
      "2025-01-01T00:00:00Z", "2025-01-02T00:00:00Z",
  });
  const auto singleton = make_index("2025-01-01T00:00:00Z");
  const auto empty = make_index(std::vector<std::int64_t>{});

  REQUIRE(descending.is_monotonic_decreasing());
  REQUIRE_FALSE(ascending.is_monotonic_decreasing());
  REQUIRE(singleton.is_monotonic_decreasing());
  REQUIRE(empty.is_monotonic_decreasing());
}

TEST_CASE("TimeIndex uniqueness detects separated duplicates on unordered axes",
          "[timeindex][unique]") {
  const auto separated = make_index({
      "2025-01-01T02:00:00Z",
      "2025-01-01T00:00:00Z",
      "2025-01-01T01:00:00Z",
      "2025-01-01T02:00:00Z",
  }, false);
  const auto distinct = make_index({
      "2025-01-01T02:00:00Z",
      "2025-01-01T00:00:00Z",
      "2025-01-01T01:00:00Z",
  }, false);

  REQUIRE_FALSE(separated.is_unique());
  REQUIRE(distinct.is_unique());
}

TEST_CASE("TimeIndex resample extrema and endpoints skip floating NaN values",
          "[timeindex][resample]") {
  const auto index = make_index({
      "2025-01-01T00:00:00Z",
      "2025-01-01T00:10:00Z",
      "2025-01-01T00:20:00Z",
      "2025-01-01T02:00:00Z",
  });
  const auto values = pluss::make_series(std::vector<double>{
      std::numeric_limits<double>::quiet_NaN(), 3.0, 1.0, 7.0});

  const auto mins = index.resample_min(values, "H").to_vec<double>();
  const auto maxs = index.resample_max(values, "H").to_vec<double>();
  const auto first = index.resample_first(values, "H").to_vec<double>();
  const auto last = index.resample_last(values, "H").to_vec<double>();

  REQUIRE(mins[0] == 1.0);
  REQUIRE(maxs[0] == 3.0);
  REQUIRE(first[0] == 3.0);
  REQUIRE(last[0] == 1.0);
  require_nan(mins[1]);
  require_nan(maxs[1]);
  require_nan(first[1]);
  require_nan(last[1]);
  REQUIRE(mins[2] == 7.0);
  REQUIRE(maxs[2] == 7.0);
  REQUIRE(first[2] == 7.0);
  REQUIRE(last[2] == 7.0);
}

TEST_CASE("TimeIndex integer resampling represents empty bins as floating NaN",
          "[timeindex][resample]") {
  const auto index = make_index({
      "2025-01-01T00:00:00Z",
      "2025-01-01T02:00:00Z",
  });
  const auto values = pluss::make_series(std::vector<std::int64_t>{4, 9});

  for (const auto& result : {
           index.resample_min(values, "H"),
           index.resample_max(values, "H"),
           index.resample_first(values, "H"),
           index.resample_last(values, "H")}) {
    REQUIRE(result.scalar_type() == pluss::ScalarType::Float64);
    const auto out = result.to_vec<double>();
    REQUIRE(out[0] == 4.0);
    require_nan(out[1]);
    REQUIRE(out[2] == 9.0);
  }
}

TEST_CASE("TimeIndex resample options control closed edges and labels",
          "[timeindex][resample]") {
  const auto index = make_index({
      "2025-01-01T00:00:00Z",
      "2025-01-01T01:00:00Z",
      "2025-01-01T02:00:00Z",
  });
  const auto values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0});
  pluss::Timeline::ResampleOptions options;
  options.freq = "H";
  options.closed = pluss::Timeline::ResampleClosed::Right;
  options.label = pluss::Timeline::ResampleLabel::Right;

  REQUIRE(index.resample_first(values, options).to_vec<double>() ==
          std::vector<double>{1.0, 3.0});
  REQUIRE(index.resample_last(values, options).to_vec<double>() ==
          std::vector<double>{2.0, 3.0});
  REQUIRE(index.resample_min(values, options).to_vec<double>() ==
          std::vector<double>{1.0, 3.0});
  REQUIRE(index.resample_max(values, options).to_vec<double>() ==
          std::vector<double>{2.0, 3.0});
  REQUIRE(pluss::timeline::format(index.resample_bins(options).labels,
                                  "%FT%T.%f%z").vector() ==
          std::vector<std::string_view>{
              "2025-01-01T01:00:00.000000000+00:00",
              "2025-01-01T02:00:00.000000000+00:00"});
}

TEST_CASE("TimeIndex resample endpoint methods preserve dtype without empty bins",
          "[timeindex][resample]") {
  const auto index = make_index({
      "2025-01-01T00:00:00Z",
      "2025-01-01T00:30:00Z",
      "2025-01-01T01:00:00Z",
  });
  const auto values = pluss::make_series(std::vector<std::int64_t>{4, 2, 9});

  REQUIRE(index.resample_min(values, "H").to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{2, 9});
  REQUIRE(index.resample_max(values, "H").to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{4, 9});
  REQUIRE(index.resample_first(values, "H").to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{4, 9});
  REQUIRE(index.resample_last(values, "H").to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{2, 9});
}

TEST_CASE("TimeIndex resample methods reject invalid alignment and ordering",
          "[timeindex][resample]") {
  const auto ordered = make_index({
      "2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z",
  });
  const auto unordered = make_index({
      "2025-01-01T01:00:00Z", "2025-01-01T00:00:00Z",
  }, false);
  const auto values = pluss::make_series(std::vector<double>{1.0, 2.0});

  REQUIRE_THROWS(ordered.resample_min(pluss::make_series({1.0}), "H"));
  REQUIRE_THROWS(ordered.resample_max(pluss::make_series({{1.0, 2.0}}), "H"));
  REQUIRE_THROWS(ordered.resample_first(values, "H", 0));
  REQUIRE_THROWS(ordered.resample_last(values, "fortnight"));
  REQUIRE_THROWS(unordered.resample_min(values, "H"));
}

TEST_CASE("TimeIndex named rolling reductions use valid values in inclusive windows",
          "[timeindex][rolling]") {
  const auto index = make_index({
      "2025-01-01T00:00:00Z", "2025-01-01T00:20:00Z",
      "2025-01-01T00:40:00Z", "2025-01-01T01:00:00Z",
      "2025-01-01T01:20:00Z",
  });
  const auto values = pluss::make_series(std::vector<double>{
      std::numeric_limits<double>::quiet_NaN(), 8.0, 2.0, 9.0, 4.0});
  const auto window = pluss::duration::minutes(45);

  const auto means = index.rolling_mean(values, window).to_vec<double>();
  const auto mins = index.rolling_min(values, window).to_vec<double>();
  const auto maxs = index.rolling_max(values, window).to_vec<double>();
  const auto first = index.rolling_first(values, window).to_vec<double>();
  const auto last = index.rolling_last(values, window).to_vec<double>();

  require_nan(means[0]);
  require_nan(mins[0]);
  require_nan(maxs[0]);
  require_nan(first[0]);
  require_nan(last[0]);
  REQUIRE(means[1] == 8.0);
  REQUIRE(means[2] == 5.0);
  REQUIRE(std::abs(means[3] - 19.0 / 3.0) < 1e-12);
  REQUIRE(means[4] == 5.0);
  REQUIRE(std::vector<double>(mins.begin() + 1, mins.end()) ==
          std::vector<double>{8.0, 2.0, 2.0, 2.0});
  REQUIRE(std::vector<double>(maxs.begin() + 1, maxs.end()) ==
          std::vector<double>{8.0, 8.0, 9.0, 9.0});
  REQUIRE(std::vector<double>(first.begin() + 1, first.end()) ==
          std::vector<double>{8.0, 8.0, 8.0, 2.0});
  REQUIRE(std::vector<double>(last.begin() + 1, last.end()) ==
          std::vector<double>{8.0, 2.0, 9.0, 4.0});
}

TEST_CASE("TimeIndex integer rolling mean promotes while endpoint reducers preserve dtype",
          "[timeindex][rolling]") {
  const auto index = make_index({
      "2025-01-01T00:00:00Z", "2025-01-01T00:20:00Z",
      "2025-01-01T00:40:00Z",
  });
  const auto values = pluss::make_series(std::vector<std::int64_t>{8, 2, 9});
  const auto window = pluss::duration::minutes(30);

  const auto mean = index.rolling_mean(values, window);
  REQUIRE(mean.scalar_type() == pluss::ScalarType::Float64);
  REQUIRE(mean.to_vec<double>() == std::vector<double>{8.0, 5.0, 5.5});
  REQUIRE(index.rolling_first(values, window).to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{8, 8, 2});
  REQUIRE(index.rolling_last(values, window).to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{8, 2, 9});
}

TEST_CASE("TimeIndex named rolling reductions reject ambiguous contracts",
          "[timeindex][rolling]") {
  const auto ordered = make_index({
      "2025-01-01T00:00:00Z", "2025-01-01T00:20:00Z",
  });
  const auto unordered = make_index({
      "2025-01-01T00:20:00Z", "2025-01-01T00:00:00Z",
  }, false);
  const auto values = pluss::make_series(std::vector<double>{1.0, 2.0});

  REQUIRE_THROWS(ordered.rolling_min(pluss::make_series({1.0}),
                                     pluss::duration::minutes(30)));
  REQUIRE_THROWS(ordered.rolling_max(values, pluss::duration::minutes(0)));
  REQUIRE_THROWS(ordered.rolling_first(
      values, pluss::duration::minutes({15, 30})));
  REQUIRE_THROWS(unordered.rolling_last(values,
                                        pluss::duration::minutes(30)));
  REQUIRE_THROWS(ordered.rolling_agg(
      values, pluss::duration::minutes(30), "median"));
}
