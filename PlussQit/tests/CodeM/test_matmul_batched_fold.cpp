// Batched matmul must equal the per-page products for every batch layout, including the shapes that
// are folded into a single GEMM (libtorch's should_fold: [..., n, k] @ [k, p] and its transpose
// [n, k] @ [..., k, p]) and the equal-batch direct-GEMM path.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

namespace {
using pluss::ScalarType;
using pluss::Series;

Series ramp(std::vector<std::int64_t> shape, ScalarType dt, double seed) {
  std::int64_t n = 1;
  for (auto s : shape) n *= s;
  std::vector<double> values(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) values[static_cast<std::size_t>(i)] = std::sin(seed + 0.37 * static_cast<double>(i)) + 0.1 * static_cast<double>(i % 7);
  return pluss::make_series(values, shape).to(dt);
}

// reference: loop over every batch index with 2-D products (the semantics matmul must preserve)
Series reference(const Series &a, const Series &b) {
  const auto bs = [](const Series &x) { return std::vector<std::int64_t>(x.sizes().begin(), x.sizes().end() - 2); };
  const auto batch_a = bs(a), batch_b = bs(b);
  std::vector<std::int64_t> batch;
  const std::size_t rank = std::max(batch_a.size(), batch_b.size());
  for (std::size_t i = 0; i < rank; ++i) {
    const std::int64_t da = i + batch_a.size() >= rank ? batch_a[i + batch_a.size() - rank] : 1;
    const std::int64_t db = i + batch_b.size() >= rank ? batch_b[i + batch_b.size() - rank] : 1;
    batch.push_back(std::max(da, db));
  }
  std::int64_t count = 1;
  for (auto s : batch) count *= s;
  const std::int64_t n = a.size(-2), p = b.size(-1);
  std::vector<Series> pages;
  for (std::int64_t flat = 0; flat < count; ++flat) {
    std::int64_t rem = flat;
    std::vector<std::int64_t> idx(batch.size());
    for (std::size_t i = batch.size(); i-- > 0;) { idx[i] = rem % batch[i]; rem /= batch[i]; }
    const auto page = [&](const Series &x, const std::vector<std::int64_t> &xb) {
      Series cur = x;
      for (std::size_t i = 0; i < xb.size(); ++i) {
        const std::size_t out_i = i + batch.size() - xb.size();
        cur = cur.select(0, xb[i] == 1 ? 0 : idx[out_i]);
      }
      return cur;
    };
    pages.push_back(page(a, batch_a).matmul(page(b, batch_b)));
  }
  std::vector<std::int64_t> shape = batch;
  shape.push_back(n);
  shape.push_back(p);
  Series out = pages.front().unsqueeze(0);
  for (std::size_t i = 1; i < pages.size(); ++i) out = out.unite(std::vector<Series>{pages[i].unsqueeze(0)}, 0);
  return out.reshape(shape);
}

void expect_close(const Series &got, const Series &want, double tol) {
  REQUIRE(got.dim() == want.dim());
  for (std::int64_t i = 0; i < got.dim(); ++i) REQUIRE(got.size(i) == want.size(i));
  REQUIRE(got.sub(want).abs().max().item<double>() <= tol);
}
} // namespace

TEST_CASE("batched matmul equals per-page products for every layout", "[series][matmul][batched]") {
  for (const auto dt : {ScalarType::Float64, ScalarType::Float32}) {
    const double tol = dt == ScalarType::Float64 ? 1e-12 : 1e-4;
    SECTION("[b,n,k] @ [k,p] folds into one GEMM") {
      const Series a = ramp({3, 4, 5}, dt, 0.1), b = ramp({5, 6}, dt, 0.7);
      expect_close(a.matmul(b), reference(a, b), tol);
      const Series a4 = ramp({2, 3, 4, 5}, dt, 0.3);
      expect_close(a4.matmul(b), reference(a4, b), tol);
    }
    SECTION("[b,n,k] @ [k] folds into one GEMM") {
      const Series a = ramp({3, 4, 5}, dt, 0.2), v = ramp({5}, dt, 1.1);
      const Series out = a.matmul(v);
      REQUIRE(out.dim() == 2);
      expect_close(out, reference(a, v.unsqueeze(-1)).squeeze(-1), tol);
    }
    SECTION("[n,k] @ [b,k,p] folds into one GEMM") {
      const Series a = ramp({4, 5}, dt, 0.4), b = ramp({3, 5, 6}, dt, 0.9);
      expect_close(a.matmul(b), reference(a, b), tol);
      const Series b4 = ramp({2, 3, 5, 6}, dt, 0.2);
      expect_close(a.matmul(b4), reference(a, b4), tol);
    }
    SECTION("equal batch shapes take the direct GEMM path") {
      const Series a = ramp({4, 3, 5}, dt, 0.5), b = ramp({4, 5, 2}, dt, 0.6);
      expect_close(a.matmul(b), reference(a, b), tol);
      const Series a4 = ramp({2, 3, 4, 5}, dt, 0.5), b4 = ramp({2, 3, 5, 2}, dt, 0.6);
      expect_close(a4.matmul(b4), reference(a4, b4), tol);
    }
    SECTION("broadcast batches still work") {
      const Series a = ramp({1, 4, 5}, dt, 0.5), b = ramp({3, 5, 2}, dt, 0.6);
      expect_close(a.matmul(b), reference(a, b), tol);
    }
    SECTION("non-contiguous operands") {
      const Series a = ramp({5, 4, 3}, dt, 0.8).transpose(0, 1); // [4,5,3] non-contiguous
      const Series b = ramp({3, 6}, dt, 0.1);
      expect_close(a.matmul(b), reference(a, b), tol);
    }
  }
}

TEST_CASE("folded matmul is fast for many tiny lanes", "[series][matmul][perf]") {
  // 20000 lanes of [1, 10] @ [10, 29]: used to run 20000 separate 2-D products
  const Series lanes = ramp({20000, 1, 10}, ScalarType::Float64, 0.3);
  const Series m = ramp({10, 29}, ScalarType::Float64, 0.9);
  const auto t0 = std::chrono::steady_clock::now();
  const Series out = lanes.matmul(m);
  const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  REQUIRE(out.size(0) == 20000);
  REQUIRE(out.size(1) == 1);
  REQUIRE(out.size(2) == 29);
  REQUIRE(ms < 250.0); // the page loop took several seconds
}
