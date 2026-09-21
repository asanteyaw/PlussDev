#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::Series;
using pluss::ScalarType;
using pluss::make_series;

// Series::grouped_mode had no reverse-mode node at all (a raw native-kernel
// buffer computation with nothing wired to the tape) -- caught by the
// full-surface [backward-manifest] audit and fixed in OpsInternal.cpp via
// the same value-equality-mask technique Invoke.cpp's median_flat_impl uses
// for plain median(), generalized to per-group scope. There is no libtorch
// equivalent to check against (grouped_mode is a Pluss-specific extension),
// so this pins the gradient down with hand-derived exact values instead of
// an oracle comparison.
TEST_CASE("Series::grouped_mode reverse: gradient splits evenly among within-group ties",
          "[autodiff][structural][grouped]") {
  // Group 0 = {1.0, 2.0, 2.0} -> mode 2.0, tied at indices 1 and 2.
  // Group 1 = {3.0, 5.0, 5.0} -> mode 5.0, tied at indices 4 and 5.
  Series values = make_series(std::vector<double>{1.0, 2.0, 2.0, 3.0, 5.0, 5.0}, {6});
  Series group_ids = make_series(std::vector<std::int64_t>{0, 0, 0, 1, 1, 1}, {6}).to(ScalarType::Int64);
  values.tracks_slope_(true);

  const Series out = values.grouped_mode(group_ids, 2);
  REQUIRE(out.to_vec<double>() == std::vector<double>{2.0, 5.0});

  // loss = sum(out^2) -> d(loss)/d(out) = 2*out = {4.0, 10.0}, split evenly
  // (0.5 each) across the two tied elements in each group; untied elements
  // (indices 0 and 3, the group's non-modal values) get exactly zero.
  out.mul(out).sum().reverse();
  REQUIRE(values.has_sensitivity());
  const auto g = values.sensitivity().to_vec<double>();
  const std::vector<double> expected{0.0, 2.0, 2.0, 0.0, 5.0, 5.0};
  REQUIRE(g.size() == expected.size());
  for (std::size_t i = 0; i < g.size(); ++i) {
    INFO("index " << i);
    REQUIRE(g[i] == Catch::Approx(expected[i]).margin(1e-12));
  }
}

TEST_CASE("Series::grouped_mode reverse: no ties, gradient routes fully to the single winner",
          "[autodiff][structural][grouped]") {
  // Group 0 = {1.0, 1.0, 3.0} -> mode 1.0 at indices 0,1 (still a tie -- use
  // a genuinely tie-free group below instead).
  // Group 0 = {7.0, 1.0, 1.0, 1.0} -> mode 1.0, three-way tie.
  // For an unambiguous single-winner case: group with all-distinct values
  // has every value "tied" with itself only (count 1), so gradient routes
  // entirely (undivided) to whichever element the kernel picks -- verify
  // conservation instead of a specific index (the kernel's tie-break among
  // *equal first-occurrence counts* is an implementation detail; what must
  // hold regardless is that exactly one element receives the full upstream
  // gradient and the total is conserved).
  Series values = make_series(std::vector<double>{7.0, 1.0, 4.0}, {3});
  Series group_ids = make_series(std::vector<std::int64_t>{0, 0, 0}, {3}).to(ScalarType::Int64);
  values.tracks_slope_(true);

  const Series out = values.grouped_mode(group_ids, 1);
  out.mul(3.0).sum().reverse();
  REQUIRE(values.has_sensitivity());
  const auto g = values.sensitivity().to_vec<double>();
  double total = 0.0;
  int nonzero_count = 0;
  for (double v : g) {
    total += v;
    if (v != 0.0) ++nonzero_count;
  }
  REQUIRE(nonzero_count == 1);
  REQUIRE(total == Catch::Approx(3.0).margin(1e-12));
}
