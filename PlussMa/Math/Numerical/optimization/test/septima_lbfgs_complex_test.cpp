// Complex-parameter support for Septima's OTHER algorithm -- LBFGS (the
// Adam branch was fixed and verified in septima_adam_complex_test.cpp;
// this file covers SeptimaOptions::Optimizer::Lbfgs, which needed a
// different, deeper fix).
//
// LBFGS's entire machinery (series_dot_, lbfgs_direction_,
// update_lbfgs_history_, the Armijo/curvature Wolfe checks) operates on
// ONE flattened Float64 vector built by septima_flatten_values_/
// septima_unflatten_values_ -- unlike Adam, which updates each parameter's
// own moment buffers independently, so per-buffer view_as_real_ wiring
// wasn't the right shape of fix here. Before this fix, flatten unconditionally
// called `.to(ScalarType::Float64)` on a (possibly complex) block with no
// complex handling at all. The fix instead applies view_as_real_ ONCE, at
// the flatten/unflatten boundary itself: every complex block becomes a real
// block of twice the size (real and imaginary channels as independent
// coordinates -- the standard way to treat optimizing a real-valued loss
// over a complex variable, sometimes called the "R^2n" or Wirtinger-real
// view), and every downstream LBFGS primitive needs no complex-aware logic
// of its own, since by the time anything sees the flat vector it is
// unconditionally real.
//
// Also exercises the precise per-mechanism complex constraint guards added
// to septima_validate_constraints_ alongside this fix (mirroring
// PlussAI::Minimax's identical box/equality/inequality distinction): box
// constraints reject a complex PARAMETER (no ordering over C); inequality
// residuals must be real regardless of enable_constraint_autodiff (the
// dual-ascent multiplier's .maximum(0.0) has no meaning over C); equality
// residuals need not be real UNLESS enable_constraint_autodiff is on (only
// then does the augmented-Lagrangian penalty need a real scalar reduction
// -- the dual-ascent multiplier update itself is pure add/mul).
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

using pluss::subsp::math::optim::septima;
using pluss::subsp::math::optim::SeptimaConstraints;
using pluss::subsp::math::optim::SeptimaExitFlag;
using pluss::subsp::math::optim::SeptimaLbfgsLineSearch;
using pluss::subsp::math::optim::SeptimaOptimizer;
using pluss::subsp::math::optim::SeptimaOptions;
using pluss::subsp::math::optim::SeptimaResult;
using pluss::subsp::math::optim::SeptimaSolveOptions;

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

Series complex_series(const std::vector<Complex>& values, const std::vector<std::int64_t>& shape) {
  return pluss::make_series(values, shape).to(ScalarType::Complex128);
}

std::vector<Complex> to_vec_c128(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Complex128).reshape({x.numel()}).host<Complex>();
}

void expect_vec_c_near(const std::vector<Complex>& lhs,
                       const std::vector<Complex>& rhs,
                       double atol,
                       const std::string& msg,
                       int* failures) {
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

// (|x - target|^2).sum() * 0.5 -- a real-valued loss of a complex variable
// with a known closed-form minimizer (x == target exactly), separable and
// strictly convex, so LBFGS should converge tightly from a modest start.
Series quadratic_loss(const Series& x, const Series& target) {
  const Series diff = x - target;
  return (diff.abs().pow(2.0)).sum() * 0.5;
}

// (|x|^2).sum() -- a real-valued dummy objective for the constraint-guard
// tests below, where only whether septima() throws (and when) matters, not
// the trajectory. `square(x)` would stay complex-valued for a complex x
// (z^2, not |z|^2) and break the "must not throw" cases for the wrong
// reason (a real-scalar-extraction failure unrelated to the constraint
// guard under test), so this uses the same real-valued abs()^2 as
// quadratic_loss above.
Series real_quadratic(const Series& x) {
  return (x.abs().pow(2.0)).sum();
}

SeptimaSolveOptions lbfgs_opts(SeptimaLbfgsLineSearch line_search) {
  SeptimaSolveOptions opts{};
  opts.step.optimizer = SeptimaOptimizer::Lbfgs;
  opts.step.lbfgs.learning_rate = 1.0;
  opts.step.lbfgs.history_size = 8;
  opts.step.lbfgs.line_search = line_search;
  opts.step.lbfgs.max_line_search_iterations = 20;
  opts.max_iterations = 80;
  opts.gradient_tolerance = 1e-10;
  opts.objective_tolerance = 1e-14;
  return opts;
}

} // namespace

int main() {
  int failures = 0;

  const std::vector<Complex> kStart{{2.5, -1.5}, {-0.5, 3.0}};
  const std::vector<Complex> kTarget{{0.25, 0.75}, {1.0, -0.5}};

  // 1) Plain (no line search) LBFGS must converge to the exact complex
  // minimizer from a nonzero start, without throwing.
  {
    Series start = complex_series(kStart, {2});
    const Series target = complex_series(kTarget, {2});
    const auto opts = lbfgs_opts(SeptimaLbfgsLineSearch::None);

    bool threw = false;
    SeptimaResult fit;
    try {
      fit = septima([&target](const Series& x) -> Series { return quadratic_loss(x, target); }, start, opts);
    } catch (const std::exception& e) {
      threw = true;
      std::cerr << "[FAIL] plain LBFGS complex solve threw: " << e.what() << '\n';
    }
    expect(!threw, "plain LBFGS must not throw on a complex parameter", &failures);
    if (!threw) {
      expect(fit.params[0].scalar_type() == ScalarType::Complex128,
             "plain LBFGS complex solve must preserve Complex128 dtype", &failures);
      expect_vec_c_near(to_vec_c128(fit.params[0]), kTarget, 1e-4,
                        "plain LBFGS must converge to the exact complex minimizer", &failures);
    }
  }

  // 2) Strong-Wolfe line search: same problem, the other LBFGS code path
  // (the one that exercises trial_grad_flat/gtd_new via
  // septima_flatten_values_ on autodiff-computed complex gradients).
  {
    Series start = complex_series(kStart, {2});
    const Series target = complex_series(kTarget, {2});
    auto opts = lbfgs_opts(SeptimaLbfgsLineSearch::StrongWolfe);

    bool threw = false;
    SeptimaResult fit;
    try {
      fit = septima([&target](const Series& x) -> Series { return quadratic_loss(x, target); }, start, opts);
    } catch (const std::exception& e) {
      threw = true;
      std::cerr << "[FAIL] strong-Wolfe LBFGS complex solve threw: " << e.what() << '\n';
    }
    expect(!threw, "strong-Wolfe LBFGS must not throw on a complex parameter", &failures);
    if (!threw) {
      expect(fit.params[0].scalar_type() == ScalarType::Complex128,
             "strong-Wolfe LBFGS complex solve must preserve Complex128 dtype", &failures);
      expect_vec_c_near(to_vec_c128(fit.params[0]), kTarget, 1e-4,
                        "strong-Wolfe LBFGS must converge to the exact complex minimizer", &failures);
      expect(fit.exit_flag != SeptimaExitFlag::NonFiniteEvaluation,
             "strong-Wolfe LBFGS must not report NonFiniteEvaluation for a well-posed complex solve",
             &failures);
    }
  }

  // 3) Determinism: two identical complex LBFGS runs must agree exactly,
  // matching the same property already pinned for real-dtype LBFGS in
  // septima_rigor_suite_test.cpp.
  {
    const Series target = complex_series(kTarget, {2});
    const auto opts = lbfgs_opts(SeptimaLbfgsLineSearch::None);
    const auto run_once = [&]() {
      Series start = complex_series(kStart, {2});
      return septima([&target](const Series& x) -> Series { return quadratic_loss(x, target); }, start, opts);
    };
    const auto a = run_once();
    const auto b = run_once();
    expect_vec_c_near(to_vec_c128(a.params[0]), to_vec_c128(b.params[0]), 1e-12,
                      "complex LBFGS must be deterministic across repeated runs", &failures);
    expect(std::abs(a.objective_value - b.objective_value) <= 1e-12,
           "complex LBFGS objective must be deterministic across repeated runs", &failures);
  }

  // 4) Box constraint on a complex parameter must be rejected (no ordering
  // over C), regardless of which Septima algorithm is selected -- mirrors
  // PlussAI::Minimax's identical, already-verified guard.
  {
    Series start = complex_series(kStart, {2});
    SeptimaConstraints constraints{};
    constraints.lbs = {complex_series({{0.0, 0.0}, {0.0, 0.0}}, {2})};
    auto opts = lbfgs_opts(SeptimaLbfgsLineSearch::None);
    bool threw = false;
    try {
      (void)septima([](const Series& x) -> Series { return real_quadratic(x); }, start, opts, constraints);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "box constraint on a complex parameter must be rejected", &failures);
  }

  // 5) A complex-valued inequality residual must be rejected unconditionally
  // (the dual-ascent multiplier's .maximum(0.0) has no meaning over C),
  // regardless of enable_constraint_autodiff.
  {
    Series start = complex_series(kStart, {2});
    SeptimaConstraints constraints{};
    constraints.inequalities = [](const std::vector<Series>& p) {
      return std::vector<Series>{p[0].sum()}; // complex-valued residual
    };
    auto opts = lbfgs_opts(SeptimaLbfgsLineSearch::None);
    opts.step.enable_constraint_autodiff = false;
    bool threw = false;
    try {
      (void)septima([](const Series& x) -> Series { return real_quadratic(x); }, start, opts, constraints);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "complex-valued inequality residual must be rejected even with autodiff disabled", &failures);
  }

  // 6) A complex-valued equality residual is rejected when
  // enable_constraint_autodiff is true (the augmented penalty needs a real
  // scalar reduction)...
  {
    Series start = complex_series(kStart, {2});
    SeptimaConstraints constraints{};
    constraints.equalities = [](const std::vector<Series>& p) {
      return std::vector<Series>{p[0].sum()}; // complex-valued residual
    };
    auto opts = lbfgs_opts(SeptimaLbfgsLineSearch::None);
    opts.step.enable_constraint_autodiff = true;
    bool threw = false;
    try {
      (void)septima([](const Series& x) -> Series { return real_quadratic(x); }, start, opts, constraints);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "complex-valued equality residual must be rejected when enable_constraint_autodiff is true",
           &failures);
  }

  // ...but is fine when enable_constraint_autodiff is false, since the
  // dual-ascent multiplier update alone is pure add/mul with no ordering
  // -- same distinction PlussAI::Minimax's equality constraints get (no
  // autodiff-objective path there at all).
  {
    Series start = complex_series(kStart, {2});
    SeptimaConstraints constraints{};
    constraints.equalities = [](const std::vector<Series>& p) {
      return std::vector<Series>{p[0].sum()}; // complex-valued residual
    };
    auto opts = lbfgs_opts(SeptimaLbfgsLineSearch::None);
    opts.step.enable_constraint_autodiff = false;
    opts.max_iterations = 3; // just needs to not throw at construction/step time
    bool threw = false;
    try {
      (void)septima([](const Series& x) -> Series { return real_quadratic(x); }, start, opts, constraints);
    } catch (const std::exception& e) {
      threw = true;
      std::cerr << "[FAIL] unexpected throw: " << e.what() << '\n';
    }
    expect(!threw, "complex-valued equality residual must be accepted when enable_constraint_autodiff is false",
           &failures);
  }

  if (failures != 0) {
    std::cerr << "septima_lbfgs_complex_test failed with " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "septima_lbfgs_complex_test passed\n";
  return 0;
}
