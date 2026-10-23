#include <pluss/ma.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::pCPU;
using pluss::optim::SeptimaConstraints;
using pluss::optim::SeptimaExitFlag;
using pluss::optim::SeptimaLbfgsLineSearch;
using pluss::optim::SeptimaOptimizer;
using pluss::optim::SeptimaResult;
using pluss::optim::SeptimaSolveOptions;
using pluss::optim::ParameterPack;
using pluss::optim::Trace;
using pluss::optim::log;
using pluss::optim::square;
using pluss::optim::sum;
using pluss::optim::trace;

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

Series make_vec(const std::vector<double>& values, const Config& cfg) {
  return pluss::make_series(values, {static_cast<std::int64_t>(values.size())}, cfg);
}

double as_f64_scalar(const Series& x) {
  if (!x.defined() || x.numel() != 1) throw std::runtime_error("expected scalar Series");
  return x.to(pCPU, ScalarType::Float64).item<double>();
}

std::vector<double> to_vec_f64(const Series& x) {
  return x.to(pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
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
      std::cerr << "[FAIL] " << msg << " idx=" << i << " lhs=" << lhs[i] << " rhs=" << rhs[i] << '\n';
      return;
    }
  }
}

SeptimaSolveOptions make_base_options() {
  SeptimaSolveOptions opts{};
  opts.max_iterations = 200;
  opts.max_no_progress_iterations = 20;
  opts.objective_tolerance = 1e-14;
  opts.gradient_tolerance = 1e-9;
  opts.step_tolerance = 1e-12;
  opts.constraint_tolerance = 1e-8;
  opts.step.adam.learning_rate = 0.08;
  opts.step.adam.weight_decay = 1e-4;
  opts.step.dual_learning_rate = 0.1;
  opts.step.equality_penalty = 10.0;
  opts.step.inequality_penalty = 10.0;
  opts.step.project_bounds = true;
  opts.step.enable_constraint_autodiff = true;
  return opts;
}

SeptimaConstraints make_simple_constraints(const Config& cfg) {
  SeptimaConstraints c{};
  c.equalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].sum() - 1.0};
  };
  c.equalities_trace = [](const std::vector<Trace>& p) {
    return std::vector<Trace>{sum(p[0]) - 1.0};
  };
  c.inequalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].square().sum() - 1.0};
  };
  c.inequalities_trace = [](const std::vector<Trace>& p) {
    return std::vector<Trace>{sum(square(p[0])) - 1.0};
  };
  c.lbs = {make_vec({0.0, 0.0}, cfg)};
  c.ubs = {make_vec({1.0, 1.0}, cfg)};
  return c;
}

} // namespace

int main() {
  int failures = 0;
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);

  // 1) Overload parity: vector objective vs ParameterPack objective.
  {
    const Series w0 = make_vec({2.0, -1.0}, cfg);
    const Series b0 = make_vec({3.0}, cfg);
    const Series wt = make_vec({1.0, 2.0}, cfg);
    const Series bt = make_vec({-0.5}, cfg);
    SeptimaSolveOptions opts = make_base_options();
    opts.max_iterations = 120;

    ParameterPack<Series> initial({{"w", w0}, {"b", b0}});
    const auto via_pack = pluss::optim::septima(
        [wt, bt](const ParameterPack<Trace>& p) -> Trace {
          return (sum(square(p.at("w") - trace(wt, false))) + sum(square(p.at("b") - trace(bt, false)))) * 0.5;
        },
        initial,
        opts);

    const auto via_vector = pluss::optim::septima(
        [wt, bt](const std::vector<Trace>& p) -> Trace {
          return (sum(square(p[0] - trace(wt, false))) + sum(square(p[1] - trace(bt, false)))) * 0.5;
        },
        std::vector<Series>{w0, b0},
        opts);

    expect_vec_near(
        to_vec_f64(via_pack.params[0]),
        to_vec_f64(via_vector.params[0]),
        1e-9,
        "septima overload parity (w)",
        &failures);
    expect_vec_near(
        to_vec_f64(via_pack.params[1]),
        to_vec_f64(via_vector.params[1]),
        1e-9,
        "septima overload parity (b)",
        &failures);
  }

  // 2) Constrained convergence for ADAM and LBFGS.
  {
    const Series init = make_vec({0.95, 0.05}, cfg);
    const Series target = make_vec({0.3, 0.7}, cfg);
    const auto objective = [target](const Trace& x) -> Trace { return sum(square(x - trace(target, false))) * 0.5; };
    const SeptimaConstraints constraints = make_simple_constraints(cfg);

    SeptimaSolveOptions adam = make_base_options();
    adam.max_iterations = 220;
    adam.step.optimizer = SeptimaOptimizer::Adam;
    const SeptimaResult adam_fit = pluss::optim::septima(objective, init, adam, constraints);
    expect(adam_fit.constraint_violation <= 5e-4, "ADAM constrained run should satisfy constraints", &failures);
    expect(std::isfinite(adam_fit.objective_value), "ADAM constrained objective should be finite", &failures);

    SeptimaSolveOptions lbfgs = make_base_options();
    lbfgs.max_iterations = 180;
    lbfgs.step.optimizer = SeptimaOptimizer::Lbfgs;
    lbfgs.step.lbfgs.learning_rate = 0.25;
    lbfgs.step.lbfgs.history_size = 10;
    const SeptimaResult lbfgs_fit = pluss::optim::septima(objective, init, lbfgs, constraints);
    expect(lbfgs_fit.constraint_violation <= 5e-4, "LBFGS constrained run should satisfy constraints", &failures);
    expect(std::isfinite(lbfgs_fit.objective_value), "LBFGS constrained objective should be finite", &failures);
  }

  // 2b) Regression: view copy_ on an initially untracked staged buffer should
  // remain connected to traced inputs (select(...).copy_ pattern).
  {
    const Series init = pluss::make_series(
        std::vector<double>{0.3, -0.1, 0.2, -0.5, 0.4, 0.7},
        {2, 3},
        cfg);
    pluss::subsp::math::diff::DerivativePolicy policy{};
    policy.strict_dependency = true;
    policy.warn_on_disconnected = false;

    bool connected = true;
    try {
      const auto [value, grads] = pluss::subsp::math::diff::value_and_grad(
          [](const std::vector<Trace>& xs) -> Trace {
            const Series x = xs[0].value();
            Series staged = pluss::series::empty_like(x);
            staged.select(1, 0).copy_(x.select(1, 0).mul(2.0));
            staged.select(1, 1).copy_(x.select(1, 1).square());
            staged.select(1, 2).copy_(x.select(1, 2).add(1.0));
            return Trace(staged.sum());
          },
          std::vector<Series>{init},
          policy);
      expect(value.defined() && value.numel() == 1, "view copy_ regression value should be scalar", &failures);
      expect(grads.size() == 1 && grads[0].defined(), "view copy_ regression gradient should be defined", &failures);
      expect(as_f64_scalar(grads[0].abs().sum()) > 0.0,
             "view copy_ regression gradient should be non-zero",
             &failures);
    } catch (const std::exception&) {
      connected = false;
    }
    expect(connected, "view copy_ regression should stay connected under strict dependency", &failures);
  }

  // 2c) End-to-end septima path with Series objective + staged in-place writes.
  {
    const Series init = pluss::make_series(
        std::vector<double>{0.25, -0.15, 0.20, -0.30, 0.40, 0.10},
        {2, 3},
        cfg);
    const Series target = pluss::make_series(
        std::vector<double>{0.8, 0.3, -0.1, -0.2, 0.5, 0.7},
        {2, 3},
        cfg);

    SeptimaSolveOptions opts = make_base_options();
    opts.max_iterations = 90;
    opts.step.optimizer = SeptimaOptimizer::Adam;
    opts.step.adam.learning_rate = 0.12;
    opts.step.enable_constraint_autodiff = true;

    SeptimaConstraints c{};
    c.inequalities = [](const std::vector<Series>& p) {
      return std::vector<Series>{p[0].select(1, 0).square() + p[0].select(1, 1).square() - 4.0};
    };

    const auto fit = pluss::optim::septima(
        [target](const Series& x) -> Series {
          Series staged = pluss::series::zeros_like(x);
          for (std::int64_t t = 0; t < x.size(1); ++t) {
            staged.select(1, t).copy_(x.select(1, t));
          }
          return (staged - target).square().sum();
        },
        init,
        opts,
        c);

    expect(std::isfinite(fit.objective_value), "series-objective septima objective should be finite", &failures);
    expect(fit.step_inf_norm > 0.0, "series-objective septima should take at least one non-zero step", &failures);
    expect(fit.params.size() == 1 && fit.params[0].defined(), "series-objective septima should return parameter block", &failures);
  }

  // 3) Progress hook stop semantics.
  {
    const Series init = make_vec({4.0, -4.0}, cfg);
    const Series target = make_vec({1.0, -2.0}, cfg);
    SeptimaSolveOptions opts = make_base_options();
    opts.max_iterations = 80;
    opts.progress_hook = [](const pluss::optim::SeptimaProgressSnapshot& s) { return s.iteration < 7; };

    const auto fit = pluss::optim::septima(
        [target](const Trace& x) -> Trace { return sum(square(x - trace(target, false))) * 0.5; },
        init,
        opts);
    expect(fit.exit_flag == SeptimaExitFlag::Stopped, "progress hook should stop septima", &failures);
    expect(fit.iteration == 7, "progress hook stop should occur at configured iteration", &failures);
  }

  // 4) Non-finite objective handling.
  {
    const Series init = make_vec({0.0}, cfg);
    SeptimaSolveOptions opts = make_base_options();
    opts.max_iterations = 10;
    opts.stop_on_nonfinite_objective = true;

    const auto fit = pluss::optim::septima(
        [](const Trace& x) -> Trace {
          return sum(log(x - 2.0));
        },
        init,
        opts);
    expect(fit.exit_flag == SeptimaExitFlag::NonFiniteEvaluation,
           "non-finite objective should map to NonFiniteEvaluation",
           &failures);
  }

  // 5) No-progress classification.
  {
    const Series init = make_vec({1.0}, cfg);
    SeptimaSolveOptions opts = make_base_options();
    opts.max_iterations = 30;
    opts.max_no_progress_iterations = 3;
    opts.gradient_tolerance = 0.0;
    opts.step_tolerance = 1e-12;
    opts.objective_tolerance = 1e-6;
    opts.step.adam.learning_rate = 1e-18;
    opts.step.optimizer = SeptimaOptimizer::Adam;

    const auto fit = pluss::optim::septima(
        [](const Trace& x) -> Trace {
          return sum(square(x));
        },
        init,
        opts);
    expect(fit.exit_flag == SeptimaExitFlag::NoProgress, "tiny-step regime should trigger NoProgress", &failures);
  }

  // 6) Determinism across repeated runs.
  {
    const Series init = make_vec({2.5, -1.5}, cfg);
    const Series target = make_vec({0.25, 0.75}, cfg);
    SeptimaSolveOptions opts = make_base_options();
    opts.max_iterations = 60;
    opts.step.optimizer = SeptimaOptimizer::Lbfgs;
    opts.step.lbfgs.learning_rate = 0.2;
    opts.step.lbfgs.history_size = 8;

    const auto run_once = [&]() {
      return pluss::optim::septima(
          [target](const Trace& x) -> Trace { return sum(square(x - trace(target, false))) * 0.5; },
          init,
          opts);
    };
    const auto a = run_once();
    const auto b = run_once();

    expect_vec_near(to_vec_f64(a.params[0]), to_vec_f64(b.params[0]), 1e-12, "septima deterministic params", &failures);
    expect(std::abs(a.objective_value - b.objective_value) <= 1e-12,
           "septima deterministic objective",
           &failures);
  }

  // 7) LBFGS strong-wolfe line-search should increase function eval count.
  {
    const Series init = make_vec({2.5, -1.5}, cfg);
    const Series target = make_vec({0.25, 0.75}, cfg);
    SeptimaSolveOptions opts = make_base_options();
    opts.max_iterations = 30;
    opts.step.optimizer = SeptimaOptimizer::Lbfgs;
    opts.step.lbfgs.learning_rate = 1.0;
    opts.step.lbfgs.history_size = 8;
    opts.step.lbfgs.max_line_search_iterations = 12;
    opts.step.lbfgs.line_search = SeptimaLbfgsLineSearch::StrongWolfe;

    const auto fit = pluss::optim::septima(
        [target](const Trace& x) -> Trace { return sum(square(x - trace(target, false))) * 0.5; },
        init,
        opts);
    expect(fit.function_evaluations > fit.iteration,
           "strong-wolfe LBFGS should perform additional function evaluations",
           &failures);
  }

  // 8) Option validation failures.
  {
    // Series-only constraint callbacks should be sufficient when constraint autodiff is enabled.
    const Series init = make_vec({0.95, 0.05}, cfg);
    const Series target = make_vec({0.30, 0.70}, cfg);

    SeptimaSolveOptions opts = make_base_options();
    opts.max_iterations = 180;
    opts.step.enable_constraint_autodiff = true;

    SeptimaConstraints c{};
    c.equalities = [](const std::vector<Series>& p) {
      return std::vector<Series>{p[0].sum() - 1.0};
    };
    c.inequalities = [](const std::vector<Series>& p) {
      return std::vector<Series>{p[0].square().sum() - 1.0};
    };
    c.lbs = {make_vec({0.0, 0.0}, cfg)};
    c.ubs = {make_vec({1.0, 1.0}, cfg)};

    const auto fit = pluss::optim::septima(
        [target](const Trace& x) -> Trace {
          return sum(square(x - trace(target, false))) * 0.5;
        },
        init,
        opts,
        c);

    expect(std::isfinite(fit.objective_value),
           "series-only constraints should run with constraint autodiff",
           &failures);
    expect(fit.constraint_violation <= 5e-4,
           "series-only constraints should satisfy bounds and residuals",
           &failures);
  }

  // 9) Series-only constraints should also work when constraint autodiff is disabled.
  {
    const Series init = make_vec({0.95, 0.05}, cfg);
    const Series target = make_vec({0.30, 0.70}, cfg);

    SeptimaSolveOptions opts = make_base_options();
    opts.max_iterations = 120;
    opts.step.enable_constraint_autodiff = false;
    opts.step.optimizer = SeptimaOptimizer::Adam;

    SeptimaConstraints c{};
    c.equalities = [](const std::vector<Series>& p) {
      return std::vector<Series>{p[0].sum() - 1.0};
    };
    c.inequalities = [](const std::vector<Series>& p) {
      return std::vector<Series>{p[0].square().sum() - 1.0};
    };
    c.lbs = {make_vec({0.0, 0.0}, cfg)};
    c.ubs = {make_vec({1.0, 1.0}, cfg)};

    const auto fit = pluss::optim::septima(
        [target](const Trace& x) -> Trace {
          return sum(square(x - trace(target, false))) * 0.5;
        },
        init,
        opts,
        c);

    expect(std::isfinite(fit.objective_value),
           "series-only constraints should run with constraint autodiff disabled",
           &failures);
    expect(fit.constraint_violation <= 1e-3,
           "series-only constraints should remain enforceable without constraint autodiff",
           &failures);
  }

  // 10) Option validation failures.
  {
    bool threw_dual = false;
    try {
      SeptimaSolveOptions bad = make_base_options();
      bad.step.dual_learning_rate = -1.0;
      (void)pluss::optim::septima(
          [](const Trace& x) -> Trace { return sum(square(x)); },
          make_vec({1.0}, cfg),
          bad);
    } catch (const std::exception&) {
      threw_dual = true;
    }
    expect(threw_dual, "septima should throw on invalid dual_learning_rate", &failures);

    bool threw_iters = false;
    try {
      SeptimaSolveOptions bad = make_base_options();
      bad.max_iterations = 0;
      (void)pluss::optim::septima(
          [](const Trace& x) -> Trace { return sum(square(x)); },
          make_vec({1.0}, cfg),
          bad);
    } catch (const std::exception&) {
      threw_iters = true;
    }
    expect(threw_iters, "septima should throw on invalid max_iterations", &failures);
  }

  // 8) Stationarity accounts for constraints: a problem solved against an active bound converges, and without
  //    constraints the stationarity is the objective gradient itself.
  {
    const Series target = make_vec({2.0, 1.0}, cfg);
    for (const auto optimizer : {SeptimaOptimizer::Adam, SeptimaOptimizer::Lbfgs}) {
      SeptimaSolveOptions opts{};  // library defaults
      opts.max_iterations = 2000;
      opts.step.optimizer = optimizer;
      if (optimizer == SeptimaOptimizer::Adam) opts.step.adam.learning_rate = 0.05;
      SeptimaConstraints boxed;
      boxed.lbs = {make_vec({0.0, 0.0}, cfg)};
      boxed.ubs = {make_vec({1.5, 1.5}, cfg)};
      const auto objective = [target](const Trace& x) -> Trace { return sum(square(x - trace(target, false))) * 0.5; };
      const auto fit = pluss::optim::septima(objective, make_vec({0.0, 0.0}, cfg), opts, boxed);
      expect(fit.exit_flag == SeptimaExitFlag::Converged, "an active bound must not prevent convergence", &failures);
      expect_vec_near(to_vec_f64(fit.params[0]), {1.5, 1.0}, 1e-6, "boxed septima solution", &failures);
      expect(fit.gradient_inf_norm > 0.4, "the objective gradient is nonzero at a bound-constrained optimum", &failures);
      expect(fit.stationarity <= opts.gradient_tolerance, "projected stationarity vanishes at the optimum", &failures);

      const auto free_fit = pluss::optim::septima(objective, make_vec({0.0, 0.0}, cfg), opts);
      expect(free_fit.exit_flag == SeptimaExitFlag::Converged, "unconstrained septima converges", &failures);
      expect(std::abs(free_fit.stationarity - free_fit.gradient_inf_norm) <= 1e-12,
             "without constraints stationarity equals the objective gradient", &failures);
    }
  }

  // 9) An inequality multiplier built up while the iterate was far outside the feasible set relaxes once the
  //    constraint is slack: from a start far outside the unit disc, Adam must still reach the projection of the
  //    target (2, 1) onto the disc, (2, 1) / sqrt(5), with the multiplier (sqrt(5) - 1) / 2.
  {
    const Series target = make_vec({2.0, 1.0}, cfg);
    SeptimaSolveOptions opts{};
    opts.max_iterations = 4000;
    opts.gradient_tolerance = 1e-5;
    opts.step_tolerance = 1e-5;
    opts.step.adam.learning_rate = 0.02;
    opts.step.inequality_penalty = 10.0;  // the multiplier step is the library default (the penalty)
    SeptimaConstraints disc;
    disc.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].square().sum() - 1.0}; };
    const auto fit = pluss::optim::septima(
        [target](const Trace& x) -> Trace { return sum(square(x - trace(target, false))) * 0.5; },
        make_vec({-2.0, 1.5}, cfg), opts, disc);
    expect_vec_near(to_vec_f64(fit.params[0]), {2.0 / std::sqrt(5.0), 1.0 / std::sqrt(5.0)}, 2e-3,
                    "inequality solution from a far-outside start", &failures);
    expect(std::abs(fit.state.lambda_inequalities[0].item<double>() - (std::sqrt(5.0) - 1.0) / 2.0) < 5e-3,
           "inequality multiplier relaxes to its true value", &failures);
  }

  // 10) L-BFGS under constraints, with and without the line search, from an easy and a far-outside start. These used to
  //     stall (the multiplier update never ran once the step was zero) or be driven far from the answer.
  {
    const Series target = make_vec({2.0, 1.0}, cfg);
    const auto objective = [target](const Trace& x) -> Trace { return sum(square(x - trace(target, false))) * 0.5; };
    for (const auto line_search : {SeptimaLbfgsLineSearch::None, SeptimaLbfgsLineSearch::StrongWolfe}) {
      for (const auto& start : {std::vector<double>{0.0, 0.0}, std::vector<double>{-2.0, 1.5}}) {
        SeptimaSolveOptions opts{};
        opts.max_iterations = 4000;
        opts.gradient_tolerance = 1e-5;
        opts.step_tolerance = 1e-5;
        opts.step.optimizer = SeptimaOptimizer::Lbfgs;
        opts.step.lbfgs.line_search = line_search;
        SeptimaConstraints disc;
        disc.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].square().sum() - 1.0}; };
        const auto fit = pluss::optim::septima(objective, make_vec(start, cfg), opts, disc);
        expect_vec_near(to_vec_f64(fit.params[0]), {2.0 / std::sqrt(5.0), 1.0 / std::sqrt(5.0)}, 2e-3, "lbfgs inequality solution", &failures);
        expect(fit.constraint_violation < 1e-3, "lbfgs inequality violation", &failures);
        expect(std::abs(fit.state.lambda_inequalities[0].item<double>() - (std::sqrt(5.0) - 1.0) / 2.0) < 5e-3, "lbfgs inequality multiplier", &failures);

        SeptimaConstraints sum_to_two;
        sum_to_two.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].sum() - 2.0}; };
        const auto eq_fit = pluss::optim::septima(objective, make_vec(start, cfg), opts, sum_to_two);
        expect_vec_near(to_vec_f64(eq_fit.params[0]), {1.5, 0.5}, 1e-3, "lbfgs equality solution", &failures);
        expect(std::abs(eq_fit.state.lambda_equalities[0].item<double>() - 0.5) < 5e-3, "lbfgs equality multiplier", &failures);
      }
    }
  }

  // 11) A nonconvex problem with bounds, two linear and two nonlinear constraints, from the library defaults (including a penalty
  //     too small to start with): minimize a^2 + 2 b^2 + 3 c^2 - 4 a  with 0 <= a <= 1.5, 0 <= b, c <= 2, b + c = 1, a b = 0.3,
  //     a + b + c <= 2.5, c^2 <= 0.6. Its solution (found independently by a sequential quadratic programming method) is
  //     (1.3310, 0.2254, 0.7746) with the objective -1.6508.
  {
    SeptimaConstraints c;
    c.lbs = {make_vec({0.0}, cfg), make_vec({0.0}, cfg), make_vec({0.0}, cfg)};
    c.ubs = {make_vec({1.5}, cfg), make_vec({2.0}, cfg), make_vec({2.0}, cfg)};
    c.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[1] + x[2] - 1.0, x[0] * x[1] - 0.3}; };
    c.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0] + x[1] + x[2] - 2.5, x[2].square() - 0.6}; };
    const std::vector<Series> start = {make_vec({0.5}, cfg), make_vec({0.5}, cfg), make_vec({0.5}, cfg)};
    const auto objective = [](const std::vector<Trace>& v) -> Trace {
      return square(v[0]) + 2.0 * square(v[1]) + 3.0 * square(v[2]) - 4.0 * v[0];
    };
    for (const auto optimizer : {SeptimaOptimizer::Adam, SeptimaOptimizer::Lbfgs}) {
      SeptimaSolveOptions opts{};
      opts.max_iterations = 5000;
      opts.gradient_tolerance = 1e-5;
      opts.step_tolerance = 1e-6;
      opts.step.optimizer = optimizer;
      if (optimizer == SeptimaOptimizer::Adam) opts.step.adam.learning_rate = 0.02;
      for (const double penalty : {1.0, 10.0, 100.0}) {
        opts.step.equality_penalty = opts.step.inequality_penalty = penalty;
        const auto fit = pluss::optim::septima(objective, start, opts, c);
        expect(fit.exit_flag == SeptimaExitFlag::Converged, "constrained nonconvex problem converges", &failures);
        expect(fit.constraint_violation <= 1e-5, "constrained nonconvex problem is feasible", &failures);
        expect(std::abs(fit.objective_value + 1.6508) < 2e-3, "constrained nonconvex objective", &failures);
        expect(std::abs(fit.params[0].item<double>() - 1.3310) < 3e-3 && std::abs(fit.params[1].item<double>() - 0.2254) < 3e-3 &&
                   std::abs(fit.params[2].item<double>() - 0.7746) < 3e-3,
               "constrained nonconvex solution", &failures);
      }
    }
  }

  if (failures != 0) {
    std::cerr << "septima_rigor_suite_test failed with " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "septima_rigor_suite_test passed\n";
  return 0;
}
