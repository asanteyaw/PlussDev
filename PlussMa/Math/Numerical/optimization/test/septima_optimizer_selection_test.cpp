#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::pCPU;
using pluss::subsp::math::optim::SeptimaConstraints;
using pluss::subsp::math::optim::SeptimaLbfgsLineSearch;
using pluss::subsp::math::optim::SeptimaOptimizer;
using pluss::subsp::math::optim::SeptimaSolveOptions;

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

double as_f64_scalar(const Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pCPU, ScalarType::Float64).item<double>();
}

SeptimaSolveOptions make_default_options() {
  SeptimaSolveOptions options{};
  options.max_iterations = 80;
  options.gradient_tolerance = 1e-10;
  options.step_tolerance = 1e-12;
  options.constraint_tolerance = 1e-12;
  options.step.adam.learning_rate = 0.1;
  options.step.adam.weight_decay = 0.0;
  return options;
}

std::vector<double> to_vec_f64(const Series& x) {
  return x.to(pCPU, ScalarType::Float64).reshape({x.numel()}).host<double>();
}

void expect_vec_near(const std::vector<double>& lhs,
                     const std::vector<double>& rhs,
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
      std::cerr << "[FAIL] " << msg << " (idx=" << i << ", lhs=" << lhs[i] << ", rhs=" << rhs[i] << ")\n";
      return;
    }
  }
}

} // namespace

int main() {
  static_assert(pluss::subsp::math::optim::SeptimaOptions{}.optimizer == SeptimaOptimizer::Adam);

  int failures = 0;
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);
  const Series init = pluss::make_series(std::vector<double>{6.0, -6.0}, {2}, cfg).reshape({2});
  const Series target = pluss::make_series(std::vector<double>{1.0, -2.0}, {2}, cfg).reshape({2});
  const auto objective = [target](const Series& x) -> Series { return ((x - target).square()).sum() * 0.5; };
  const double initial_objective = as_f64_scalar((init - target).square().sum() * 0.5);

  // --- Scenario 1: ADAM default path remains default and convergent ---
  SeptimaSolveOptions adam_options = make_default_options();

  const auto adam_fit = pluss::subsp::math::optim::septima(objective, init, adam_options);
  expect(adam_fit.state.step > 0, "ADAM run should execute at least one step", &failures);
  expect(adam_fit.state.adam.step > 0, "ADAM run should update adam state", &failures);
  expect(adam_fit.objective_value < initial_objective, "ADAM run should reduce objective", &failures);

  // --- Scenario 2: Explicit LBFGS path converges and updates LBFGS state ---
  SeptimaSolveOptions lbfgs_options = adam_options;
  lbfgs_options.max_iterations = 60;
  lbfgs_options.step.optimizer = SeptimaOptimizer::Lbfgs;
  lbfgs_options.step.lbfgs.learning_rate = 0.25;
  lbfgs_options.step.lbfgs.history_size = 12;
  lbfgs_options.step.lbfgs.curvature_tolerance = 1e-12;

  const auto lbfgs_fit = pluss::subsp::math::optim::septima(objective, init, lbfgs_options);
  expect(lbfgs_fit.state.step > 0, "LBFGS run should execute at least one step", &failures);
  expect(lbfgs_fit.state.adam.step == 0, "LBFGS run should not mutate adam state", &failures);
  expect(lbfgs_fit.state.lbfgs_prev_grad_flat.defined(), "LBFGS run should track previous gradient", &failures);
  expect(lbfgs_fit.state.lbfgs_prev_step_flat.defined(), "LBFGS run should track previous step", &failures);
  expect(lbfgs_fit.state.lbfgs_s_history.size() == lbfgs_fit.state.lbfgs_y_history.size(),
         "LBFGS history buffers should stay size-aligned",
         &failures);
  expect(lbfgs_fit.state.lbfgs_s_history.size() <=
             static_cast<std::size_t>(lbfgs_options.step.lbfgs.history_size),
         "LBFGS history should respect configured memory",
         &failures);
  expect(lbfgs_fit.objective_value < initial_objective, "LBFGS run should reduce objective", &failures);

  // --- Scenario 3: Determinism / baseline-style behavior (PyTorch-style exactness intent) ---
  SeptimaSolveOptions deterministic = lbfgs_options;
  deterministic.max_iterations = 14;
  deterministic.step.lbfgs.learning_rate = 0.2;
  deterministic.step.lbfgs.history_size = 8;
  const auto lbfgs_fit_a = pluss::subsp::math::optim::septima(objective, init, deterministic);
  const auto lbfgs_fit_b = pluss::subsp::math::optim::septima(objective, init, deterministic);
  expect_vec_near(
      to_vec_f64(lbfgs_fit_a.params[0]),
      to_vec_f64(lbfgs_fit_b.params[0]),
      1e-12,
      "LBFGS deterministic replay should produce identical params",
      &failures);
  expect(std::abs(lbfgs_fit_a.objective_value - lbfgs_fit_b.objective_value) <= 1e-12,
         "LBFGS deterministic replay should produce identical objective",
         &failures);

  // --- Scenario 4: Limited-memory behavior / history cap ---
  SeptimaSolveOptions short_history = lbfgs_options;
  short_history.max_iterations = 35;
  short_history.step.lbfgs.learning_rate = 0.2;
  short_history.step.lbfgs.history_size = 2;
  const auto short_history_fit = pluss::subsp::math::optim::septima(objective, init, short_history);
  expect(short_history_fit.state.lbfgs_s_history.size() <= 2,
         "LBFGS s-history should respect history_size cap",
         &failures);
  expect(short_history_fit.state.lbfgs_y_history.size() <= 2,
         "LBFGS y-history should respect history_size cap",
         &failures);

  // --- Scenario 5: Curvature gating behavior ---
  SeptimaSolveOptions strict_curvature = lbfgs_options;
  strict_curvature.max_iterations = 20;
  strict_curvature.step.lbfgs.learning_rate = 0.25;
  strict_curvature.step.lbfgs.curvature_tolerance = 1e8;
  const auto strict_fit = pluss::subsp::math::optim::septima(objective, init, strict_curvature);
  expect(strict_fit.state.lbfgs_s_history.empty(),
         "LBFGS should reject all curvature pairs with very strict curvature_tolerance",
         &failures);
  expect(strict_fit.state.lbfgs_y_history.empty(),
         "LBFGS should reject all gradient deltas with very strict curvature_tolerance",
         &failures);

  // --- Scenario 6: Strong-Wolfe line search path ---
  SeptimaSolveOptions strong_wolfe = lbfgs_options;
  strong_wolfe.max_iterations = 60;
  strong_wolfe.step.lbfgs.learning_rate = 1.0;
  strong_wolfe.step.lbfgs.max_line_search_iterations = 12;
  strong_wolfe.step.lbfgs.line_search = SeptimaLbfgsLineSearch::StrongWolfe;
  const auto strong_wolfe_fit = pluss::subsp::math::optim::septima(objective, init, strong_wolfe);
  expect(strong_wolfe_fit.objective_value < initial_objective, "LBFGS strong-wolfe should reduce objective", &failures);
  expect(strong_wolfe_fit.function_evaluations > strong_wolfe_fit.iteration,
         "LBFGS strong-wolfe should perform extra objective evaluations",
         &failures);

  // --- Scenario 7: Multi-block path + bounds/constraints compatibility ---
  const Series w0 = pluss::make_series(std::vector<double>{3.0, -4.0}, {2}, cfg).reshape({2});
  const Series b0 = pluss::make_series(std::vector<double>{5.0}, {1}, cfg).reshape({1});
  const Series wt = pluss::make_series(std::vector<double>{1.0, 2.0}, {2}, cfg).reshape({2});
  const Series bt = pluss::make_series(std::vector<double>{-1.0}, {1}, cfg).reshape({1});
  SeptimaConstraints constraints{};
  constraints.lbs = {
      pluss::make_series(std::vector<double>{-10.0, -10.0}, {2}, cfg).reshape({2}),
      pluss::make_series(std::vector<double>{-10.0}, {1}, cfg).reshape({1}),
  };
  constraints.ubs = {
      pluss::make_series(std::vector<double>{10.0, 10.0}, {2}, cfg).reshape({2}),
      pluss::make_series(std::vector<double>{10.0}, {1}, cfg).reshape({1}),
  };
  SeptimaSolveOptions multiblock_options = lbfgs_options;
  multiblock_options.max_iterations = 50;
  multiblock_options.step.lbfgs.learning_rate = 0.2;
  const double multiblock_initial =
      as_f64_scalar(((w0 - wt).square().sum() + (b0 - bt).square().sum()) * 0.5);
  const auto multiblock_fit = pluss::subsp::math::optim::septima(
      [wt, bt](const std::vector<Series>& p) -> Series {
        return (((p[0] - wt).square()).sum() + ((p[1] - bt).square()).sum()) * 0.5;
      },
      std::vector<Series>{w0, b0},
      multiblock_options,
      constraints);
  expect(multiblock_fit.params.size() == 2, "Multi-block LBFGS should preserve parameter block count", &failures);
  expect(multiblock_fit.objective_value < multiblock_initial,
         "Multi-block LBFGS should reduce objective",
         &failures);

  // --- Scenario 8: Option validation failures ---
  {
    bool threw = false;
    try {
      SeptimaSolveOptions bad = make_default_options();
      bad.step.optimizer = SeptimaOptimizer::Lbfgs;
      bad.step.lbfgs.learning_rate = 0.0;
      (void)pluss::subsp::math::optim::septima(objective, init, bad);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "LBFGS invalid learning_rate should throw", &failures);
  }
  {
    bool threw = false;
    try {
      SeptimaSolveOptions bad = make_default_options();
      bad.step.optimizer = SeptimaOptimizer::Lbfgs;
      bad.step.lbfgs.history_size = -1;
      (void)pluss::subsp::math::optim::septima(objective, init, bad);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "LBFGS invalid history_size should throw", &failures);
  }
  {
    bool threw = false;
    try {
      SeptimaSolveOptions bad = make_default_options();
      bad.step.optimizer = SeptimaOptimizer::Lbfgs;
      bad.step.lbfgs.wolfe_c1 = 1.2;
      (void)pluss::subsp::math::optim::septima(objective, init, bad);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "LBFGS invalid wolfe_c1 should throw", &failures);
  }
  {
    bool threw = false;
    try {
      SeptimaSolveOptions bad = make_default_options();
      bad.step.optimizer = SeptimaOptimizer::Lbfgs;
      bad.step.lbfgs.wolfe_c2 = 0.0;
      (void)pluss::subsp::math::optim::septima(objective, init, bad);
    } catch (const std::exception&) {
      threw = true;
    }
    expect(threw, "LBFGS invalid wolfe_c2 should throw", &failures);
  }

  if (failures != 0) {
    std::cerr << "septima_optimizer_selection_test failed with " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "septima_optimizer_selection_test passed\n";
  return 0;
}
