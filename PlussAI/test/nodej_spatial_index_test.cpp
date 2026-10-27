#include <pluss/ai/ml.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using namespace pluss::ai::nodej;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

std::vector<double> as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
}

std::vector<std::int64_t> as_i64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Int64).reshape({x.numel()}).host<std::int64_t>();
}

Series make_points(const std::vector<double>& flat, std::int64_t n, std::int64_t dim) {
  return pluss::make_series(flat, {n, dim}, f64());
}

// Ground truth: exhaustive scan, no tree, no pruning. Returns *every*
// candidate sorted by distance (not truncated to k) -- truncating here would
// bake in an arbitrary index-order tie-break that a correct KD-tree has no
// obligation to reproduce when multiple points sit at the exact same
// distance (e.g. duplicate points).
std::vector<std::pair<double, std::int64_t>> brute_force_all(
    const std::vector<double>& points, std::int64_t n, std::int64_t dim,
    const double* query, std::int64_t exclude_index) {
  std::vector<std::pair<double, std::int64_t>> all{};
  all.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    if (i == exclude_index) continue;
    double d2 = 0.0;
    for (std::int64_t f = 0; f < dim; ++f) {
      const double dv = query[f] - points[static_cast<std::size_t>(i * dim + f)];
      d2 += dv * dv;
    }
    all.emplace_back(d2, i);
  }
  std::sort(all.begin(), all.end());
  return all;
}

bool approx(double a, double b, double tol = 1e-9) {
  return std::abs(a - b) <= tol;
}

// `got` (k results from SpatialIndex) is valid against `all` (every
// candidate, brute-force sorted) if: every got index is distinct, every got
// index's claimed distance matches its true distance, and -- the actual
// optimality bar, tolerant of tie-break order -- the farthest point in
// `got` is no farther than the (k+1)-th closest point overall (so nothing
// closer was left out, regardless of which tied points got picked).
bool k_nearest_is_valid(const std::vector<std::pair<double, std::int64_t>>& got,
                        const std::vector<std::pair<double, std::int64_t>>& all,
                        std::int64_t k) {
  if (static_cast<std::int64_t>(got.size()) != k) return false;

  const auto distance_of = [&](std::int64_t idx) -> const double* {
    for (const auto& [d2, i] : all) {
      if (i == idx) return &d2;
    }
    return nullptr;
  };

  std::vector<std::int64_t> seen{};
  seen.reserve(got.size());
  double worst_got = -1.0;
  for (const auto& [d2, idx] : got) {
    if (std::find(seen.begin(), seen.end(), idx) != seen.end()) return false;  // duplicate index
    seen.push_back(idx);
    const double* truth = distance_of(idx);
    if (truth == nullptr || !approx(*truth, d2)) return false;  // wrong distance or index not a real candidate
    worst_got = std::max(worst_got, d2);
  }

  if (static_cast<std::size_t>(k) < all.size()) {
    const double next_closest = all[static_cast<std::size_t>(k)].first;
    if (worst_got > next_closest && !approx(worst_got, next_closest)) return false;
  }
  return true;
}

bool check_query_k_matches_brute_force(std::ostream* err,
                                       const std::vector<double>& points,
                                       std::int64_t n,
                                       std::int64_t dim,
                                       const std::vector<double>& queries,
                                       std::int64_t n_queries,
                                       std::int64_t k,
                                       const char* label) {
  auto index_or = SpatialIndex::build(make_points(points, n, dim));
  if (!index_or.ok()) {
    if (err) *err << label << ": SpatialIndex::build failed\n";
    return false;
  }
  const SpatialIndex index = index_or.move_value();

  const Series query_series = make_points(queries, n_queries, dim);
  auto result_or = index.query_k(query_series, k);
  if (!result_or.ok()) {
    if (err) *err << label << ": query_k failed\n";
    return false;
  }
  const KNearestResult result = result_or.move_value();
  const auto got_idx = as_i64(result.indices);
  const auto got_dist2 = as_f64(result.distances2);
  const std::int64_t effective_k = std::min(k, n);

  for (std::int64_t qi = 0; qi < n_queries; ++qi) {
    std::vector<std::pair<double, std::int64_t>> got{};
    got.reserve(static_cast<std::size_t>(effective_k));
    for (std::int64_t j = 0; j < effective_k; ++j) {
      const std::size_t pos = static_cast<std::size_t>(qi * effective_k + j);
      got.emplace_back(got_dist2[pos], got_idx[pos]);
    }
    const auto all = brute_force_all(points, n, dim, queries.data() + qi * dim, -1);
    if (!k_nearest_is_valid(got, all, effective_k)) {
      if (err) *err << label << ": query " << qi << " does not match brute force\n";
      return false;
    }
  }
  return true;
}

bool check_query_self_k_matches_brute_force(std::ostream* err,
                                            const std::vector<double>& points,
                                            std::int64_t n,
                                            std::int64_t dim,
                                            std::int64_t k,
                                            const char* label) {
  auto index_or = SpatialIndex::build(make_points(points, n, dim));
  if (!index_or.ok()) {
    if (err) *err << label << ": SpatialIndex::build failed\n";
    return false;
  }
  const SpatialIndex index = index_or.move_value();

  auto result_or = index.query_self_k(k);
  if (!result_or.ok()) {
    if (err) *err << label << ": query_self_k failed\n";
    return false;
  }
  const KNearestResult result = result_or.move_value();
  const auto got_idx = as_i64(result.indices);
  const auto got_dist2 = as_f64(result.distances2);
  const std::int64_t effective_k = std::min(k, n - 1);

  for (std::int64_t qi = 0; qi < n; ++qi) {
    std::vector<std::pair<double, std::int64_t>> got{};
    got.reserve(static_cast<std::size_t>(effective_k));
    for (std::int64_t j = 0; j < effective_k; ++j) {
      const std::size_t pos = static_cast<std::size_t>(qi * effective_k + j);
      got.emplace_back(got_dist2[pos], got_idx[pos]);
    }
    // Self excluded outright.
    for (const auto& [d2, idx] : got) {
      if (idx == qi) {
        if (err) *err << label << ": query_self_k(" << qi << ") returned itself as a neighbor\n";
        return false;
      }
    }
    const auto all = brute_force_all(points, n, dim, points.data() + qi * dim, qi);
    if (!k_nearest_is_valid(got, all, effective_k)) {
      if (err) *err << label << ": self-query " << qi << " does not match brute force\n";
      return false;
    }
  }
  return true;
}

bool run_nodej_spatial_index_checks(std::ostream* err = nullptr) {
  std::mt19937 rng(20260804);
  std::uniform_real_distribution<double> dist(-10.0, 10.0);

  const auto random_points = [&](std::int64_t n, std::int64_t dim) {
    std::vector<double> out(static_cast<std::size_t>(n * dim));
    for (auto& v : out) v = dist(rng);
    return out;
  };

  // Small, easy to reason about by hand if this ever regresses.
  {
    const std::vector<double> points = {0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 5.0, 5.0, -3.0, 2.0};
    const std::vector<double> queries = {0.1, 0.1};
    if (!check_query_k_matches_brute_force(err, points, 5, 2, queries, 1, 2, "tiny fixed 2D")) return false;
  }

  // Random, moderate size, several (n, dim, k) combinations, both
  // cross-set query_k and self-vs-self query_self_k.
  const std::vector<std::tuple<std::int64_t, std::int64_t, std::int64_t>> cases = {
      {50, 2, 3},
      {50, 5, 7},
      {200, 3, 5},
      {200, 10, 1},
      {30, 30, 4},   // dimensionality high enough that pruning barely helps -- still must be exact
      {5, 2, 10},    // k > n, must clamp
  };
  for (const auto& [n, dim, k] : cases) {
    const auto points = random_points(n, dim);
    const auto queries = random_points(std::max<std::int64_t>(1, n / 5), dim);
    const std::int64_t n_queries = static_cast<std::int64_t>(queries.size() / static_cast<std::size_t>(dim));
    if (!check_query_k_matches_brute_force(err, points, n, dim, queries, n_queries, k, "random cross-set")) {
      return false;
    }
    if (n >= 2) {
      if (!check_query_self_k_matches_brute_force(err, points, n, dim, k, "random self-query")) {
        return false;
      }
    }
  }

  // Duplicate points (exact ties at distance 0) must not confuse
  // self-exclusion or crowd out results.
  {
    std::vector<double> points{};
    for (int i = 0; i < 10; ++i) {
      points.push_back(1.0);
      points.push_back(1.0);
    }
    points.push_back(50.0);
    points.push_back(50.0);
    if (!check_query_self_k_matches_brute_force(err, points, 11, 2, 3, "duplicate points self-query")) {
      return false;
    }
  }

  // query_self_k with n == 2 (k clamps to 1, the only "other" point).
  {
    const std::vector<double> points = {0.0, 0.0, 10.0, 10.0};
    if (!check_query_self_k_matches_brute_force(err, points, 2, 2, 5, "n=2 self-query clamp")) {
      return false;
    }
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("nodej spatial index suite", "[nodej][neighbor]") {
  if (!run_nodej_spatial_index_checks()) {
    FAIL("nodej spatial index checks failed");
  }
}
#else
int main() {
  if (!run_nodej_spatial_index_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "nodej spatial index checks passed\n";
  return 0;
}
#endif
