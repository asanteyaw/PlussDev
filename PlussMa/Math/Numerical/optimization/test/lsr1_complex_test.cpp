// Complex-parameter support for Septima's third algorithm, L-SR1 trust
// region (`optim_lsr1.h`, the user's own from-scratch implementation of
// Erway/Griffin/Marcia/Omheni's L-SR1-TR, which had no test coverage at
// all and was not yet wired into the pluss::ma umbrella include -- both
// fixed alongside the complex-support work below). No prior real-dtype
// test existed for this algorithm anywhere in the repo, so this file
// covers both baselines (real-dtype correctness) and the new
// complex-parameter support together, rather than assuming a pre-existing
// oracle to diff against.
//
// Unlike Adam (per-parameter moment buffers) and like Septima's LBFGS
// branch, L-SR1-TR is inherently a single-flat-REAL-vector method: the
// trust region itself is defined via the 2-norm on R^n, and Algorithm 2's
// subproblem solve (eigendecomposition of a k x k matrix built from the
// S/Y secant-pair history, Cholesky-based lambda_hat computation, the
// boundary/hard-case branches) fundamentally assumes a real symmetric
// Hessian approximation. Before this fix, the single-Series entry point
// cast x0 straight to Float64 with no complex handling at all (would throw
// or silently corrupt a complex parameter). The fix delegates the
// single-Series overload to the multi-block overload, which already routes
// through septima_flatten_values_/septima_unflatten_values_ -- the same
// view_as_real_/view_as_complex_ helpers fixed for Septima's LBFGS branch,
// reused here verbatim rather than re-derived. Once the flat vector reaching
// l_sr1_trust_region_core_ is unconditionally real, none of Algorithm 2's
// eigendecomposition/Cholesky machinery needs any complex-aware logic --
// exactly the same "push the fix to the boundary" shape as the LBFGS fix.
#include <pluss/ma.h>

#include <cmath>
#include <complex>
#include <iostream>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using Complex = std::complex<double>;

using pluss::subsp::math::optim::l_sr1_trust_region;
using pluss::subsp::math::optim::Lsr1ExitFlag;
using pluss::subsp::math::optim::Lsr1Result;
using pluss::subsp::math::optim::Lsr1SolveOptions;

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

Series real_series(const std::vector<double>& values, const std::vector<std::int64_t>& shape) {
  return pluss::make_series(values, shape).to(ScalarType::Float64);
}

Series complex_series(const std::vector<Complex>& values, const std::vector<std::int64_t>& shape) {
  return pluss::make_series(values, shape).to(ScalarType::Complex128);
}

std::vector<double> to_vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64)
      .reshape({x.numel()})
      .pull<double>();
}

std::vector<Complex> to_vec_c128(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Complex128)
      .reshape({x.numel()})
      .pull<Complex>();
}

void expect_vec_near(const std::vector<double>& lhs, const std::vector<double>& rhs, double atol,
                     const std::string& msg, int* failures) {
  if (lhs.size() != rhs.size()) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << " (size mismatch)\n";
    return;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (std::abs(lhs[i] - rhs[i]) > atol) {
      ++(*failures);
      std::cerr << "[FAIL] " << msg << " idx=" << i << " lhs=" << lhs[i] << " rhs=" << rhs[i] << '\n';
      return;
    }
  }
}

void expect_vec_c_near(const std::vector<Complex>& lhs, const std::vector<Complex>& rhs, double atol,
                       const std::string& msg, int* failures) {
  if (lhs.size() != rhs.size()) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << " (size mismatch)\n";
    return;
  }
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    if (std::abs(lhs[i] - rhs[i]) > atol) {
      ++(*failures);
      std::cerr << "[FAIL] " << msg << " idx=" << i << " lhs=" << lhs[i] << " rhs=" << rhs[i] << '\n';
      return;
    }
  }
}

// A weighted quadratic (different curvature per coordinate, so the SR1
// compact representation actually has nontrivial content once a few (s,y)
// pairs accumulate, rather than degenerating to isotropic steepest
// descent) with an exact closed-form minimizer at `target` regardless of
// the weights, as long as they're positive.
Series weighted_quadratic(const Series& x, const Series& target, const Series& weights) {
  const Series diff = x - target;
  return (weights * diff.square()).sum();
}

// Complex counterpart: (w_i * |x_i - target_i|^2).sum(), real-valued for a
// complex x (square() on a complex Trace would stay complex-valued, z^2
// not |z|^2 -- same distinction established throughout this session's
// complex-support work).
Series weighted_quadratic_complex(const Series& x, const Series& target, const Series& weights) {
  const Series diff = x - target;
  return (weights * diff.abs().pow(2.0)).sum();
}

Lsr1SolveOptions make_opts() {
  Lsr1SolveOptions opts{};
  opts.max_iterations = 200;
  opts.gradient_tolerance = 1e-10;
  opts.step.initial_radius = 1.0;
  opts.step.memory_size = 6;
  return opts;
}

} // namespace

int main() {
  int failures = 0;

  // 1) Real-dtype baseline, single-Series overload -- no prior test existed
  // for this algorithm at all, so this doubles as the first-ever
  // correctness check of l_sr1_trust_region itself, not just a complex
  // regression guard.
  {
    const Series target = real_series({0.25, -1.5, 3.0}, {3});
    const Series weights = real_series({1.0, 4.0, 0.25}, {3});
    const Series start = real_series({2.0, 2.0, -2.0}, {3});
    const auto opts = make_opts();

    std::function<Series(const Series&)> objective = [&](const Series& x) -> Series {
      return weighted_quadratic(x, target, weights);
    };
    const auto result = l_sr1_trust_region(objective, start, opts);

    expect(result.converged, "real single-Series SR1 must converge on a well-posed convex quadratic", &failures);
    expect(result.exit_flag == Lsr1ExitFlag::Converged, "real single-Series SR1 exit_flag must be Converged",
           &failures);
    expect_vec_near(to_vec_f64(result.params), {0.25, -1.5, 3.0}, 1e-4,
                    "real single-Series SR1 must reach the exact minimizer", &failures);
  }

  // 2) Real-dtype baseline, multi-block overload (2 blocks).
  {
    const Series target0 = real_series({0.25, -1.5}, {2});
    const Series target1 = real_series({3.0}, {1});
    const Series weights0 = real_series({1.0, 4.0}, {2});
    const Series weights1 = real_series({0.25}, {1});
    const std::vector<Series> start{real_series({2.0, 2.0}, {2}), real_series({-2.0}, {1})};
    const auto opts = make_opts();

    std::function<Series(const std::vector<Series>&)> objective = [&](const std::vector<Series>& xs) -> Series {
      return weighted_quadratic(xs[0], target0, weights0) + weighted_quadratic(xs[1], target1, weights1);
    };
    const auto [params, result] = l_sr1_trust_region(objective, start, opts);

    expect(result.converged, "real multi-block SR1 must converge on a well-posed convex quadratic", &failures);
    expect_vec_near(to_vec_f64(params[0]), {0.25, -1.5}, 1e-4, "real multi-block SR1 block 0 minimizer", &failures);
    expect_vec_near(to_vec_f64(params[1]), {3.0}, 1e-4, "real multi-block SR1 block 1 minimizer", &failures);
  }

  // 3) Complex parameter, single-Series overload (the delegating entry
  // point this fix touches directly).
  {
    const Series target = complex_series({{0.25, 0.75}, {-1.0, 1.5}}, {2});
    const Series weights = real_series({1.0, 3.0}, {2});
    const Series start = complex_series({{2.5, -1.5}, {-0.5, 3.0}}, {2});
    const auto opts = make_opts();

    std::function<Series(const Series&)> objective = [&](const Series& x) -> Series {
      return weighted_quadratic_complex(x, target, weights);
    };

    bool threw = false;
    Lsr1Result result;
    try {
      result = l_sr1_trust_region(objective, start, opts);
    } catch (const std::exception& e) {
      threw = true;
      std::cerr << "[FAIL] complex single-Series SR1 threw: " << e.what() << '\n';
    }
    expect(!threw, "complex single-Series SR1 must not throw", &failures);
    if (!threw) {
      expect(result.params.scalar_type() == ScalarType::Complex128,
             "complex single-Series SR1 must preserve Complex128 dtype", &failures);
      expect(result.converged, "complex single-Series SR1 must converge on a well-posed convex quadratic",
             &failures);
      expect_vec_c_near(to_vec_c128(result.params), {{0.25, 0.75}, {-1.0, 1.5}}, 1e-4,
                        "complex single-Series SR1 must reach the exact minimizer", &failures);
    }
  }

  // 4) Complex parameters, multi-block overload (2 blocks).
  {
    const Series target0 = complex_series({{0.25, 0.75}}, {1});
    const Series target1 = complex_series({{-1.0, 1.5}, {2.0, -0.5}}, {2});
    const Series weights0 = real_series({1.0}, {1});
    const Series weights1 = real_series({2.0, 0.5}, {2});
    const std::vector<Series> start{complex_series({{2.5, -1.5}}, {1}),
                                    complex_series({{-0.5, 3.0}, {1.0, 1.0}}, {2})};
    const auto opts = make_opts();

    std::function<Series(const std::vector<Series>&)> objective = [&](const std::vector<Series>& xs) -> Series {
      return weighted_quadratic_complex(xs[0], target0, weights0) +
             weighted_quadratic_complex(xs[1], target1, weights1);
    };

    bool threw = false;
    std::vector<Series> params;
    Lsr1Result result;
    try {
      std::tie(params, result) = l_sr1_trust_region(objective, start, opts);
    } catch (const std::exception& e) {
      threw = true;
      std::cerr << "[FAIL] complex multi-block SR1 threw: " << e.what() << '\n';
    }
    expect(!threw, "complex multi-block SR1 must not throw", &failures);
    if (!threw) {
      expect(params[0].scalar_type() == ScalarType::Complex128 && params[1].scalar_type() == ScalarType::Complex128,
             "complex multi-block SR1 must preserve Complex128 dtype in both blocks", &failures);
      expect_vec_c_near(to_vec_c128(params[0]), {{0.25, 0.75}}, 1e-4, "complex multi-block SR1 block 0 minimizer",
                        &failures);
      expect_vec_c_near(to_vec_c128(params[1]), {{-1.0, 1.5}, {2.0, -0.5}}, 1e-4,
                        "complex multi-block SR1 block 1 minimizer", &failures);
    }
  }

  // 5) Determinism: two identical complex SR1 runs must agree exactly.
  {
    const Series target = complex_series({{0.25, 0.75}, {-1.0, 1.5}}, {2});
    const Series weights = real_series({1.0, 3.0}, {2});
    const auto opts = make_opts();
    std::function<Series(const Series&)> objective = [&](const Series& x) -> Series {
      return weighted_quadratic_complex(x, target, weights);
    };
    const auto run_once = [&]() {
      Series start = complex_series({{2.5, -1.5}, {-0.5, 3.0}}, {2});
      return l_sr1_trust_region(objective, start, opts);
    };
    const auto a = run_once();
    const auto b = run_once();
    expect_vec_c_near(to_vec_c128(a.params), to_vec_c128(b.params), 1e-12,
                      "complex SR1 must be deterministic across repeated runs", &failures);
    expect(std::abs(a.objective_value - b.objective_value) <= 1e-12,
           "complex SR1 objective must be deterministic across repeated runs", &failures);
  }

  // 6) A memory larger than the number of variables must be capped at n: with n + 1 stored pairs the history matrices have more
  // columns than rows, and the old code read past the end of the reduced QR factor, so the iteration varied from run to run.
  // Ten-variable chained Rosenbrock with memory 30 must reproduce itself exactly and behave as memory 10.
  {
    const std::int64_t n = 10;
    std::vector<double> first, second, start;
    for (std::int64_t i = 0; i + 1 < n; ++i) { first.push_back(static_cast<double>(i)); second.push_back(static_cast<double>(i + 1)); }
    for (std::int64_t i = 0; i < n; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
    const Series head = pluss::make_series(first, {n - 1}).to(ScalarType::Int64);
    const Series tail = pluss::make_series(second, {n - 1}).to(ScalarType::Int64);
    auto chain = [&](const Series& x) -> Series {
      const Series a = x.index_select(0, head), b = x.index_select(0, tail);
      return (100.0 * (b - a.square()).square() + (1.0 - a).square()).sum();
    };
    auto run = [&](std::int64_t memory) {
      Lsr1SolveOptions o;
      o.max_iterations = 1000;
      o.step.memory_size = memory;
      return l_sr1_trust_region(chain, pluss::make_series(start, {n}).to(ScalarType::Float64), o);
    };
    const auto reference = run(10);
    bool identical = true;
    for (int repeat = 0; repeat < 12; ++repeat) {
      const auto r = run(30);
      identical = identical && r.exit_flag == reference.exit_flag && r.iteration == reference.iteration && r.objective_value == reference.objective_value;
    }
    if (!identical || reference.exit_flag != Lsr1ExitFlag::Converged) {
      std::cerr << "[FAIL] memory above n is capped and the iteration is reproducible\n";
      ++failures;
    }
  }

  if (failures != 0) {
    std::cerr << "lsr1_complex_test failed with " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "lsr1_complex_test passed\n";
  return 0;
}
