// pluss::subsp::math::sde -- Fluctua (fluctua/fluctua_adjoint) and the
// standalone Ito/Stratonovich stochastic-integral primitives.
//
// Verification strategy, mirroring this program's established discipline:
//  - ito_integral()/stratonovich_integral() are checked against EXACT
//    discrete algebraic identities (not approximations, not statistics):
//    for f(t, W_t) = W_t, the discrete Ito sum equals EXACTLY
//    0.5*(W_T^2 - realized_quadratic_variation), the discrete
//    Stratonovich sum equals EXACTLY 0.5*W_T^2, and their difference
//    equals EXACTLY half the realized quadratic variation -- these hold
//    for ANY step count via pure telescoping-sum algebra, so this pins
//    down the general (caller-supplied-integrand) implementation with a
//    non-statistical, non-asymptotic oracle.
//  - A batched statistical check (many independent paths in one call)
//    confirms E[integral_0^T W dW] is close to its true value 0 (the
//    martingale property / Ito isometry), with a generous
//    (~10-standard-error) tolerance to avoid flakiness while still
//    catching a genuinely broken implementation.
//  - fluctua()'s forward accuracy is checked two ways: (a) a single fixed
//    path, large step count, compared against GBM's exact closed-form
//    solution evaluated at the SAME realized Wiener increment
//    (`wiener_final`) fluctua() itself used -- demonstrating the scheme
//    converges to the right SDE solution given the same noise, not just
//    "looks plausible"; (b) a batched statistical check (many independent
//    Ornstein-Uhlenbeck paths) against OU's EXACT conditional mean/
//    variance formulas -- the standard, textbook way SDE simulators are
//    validated, since a single path's outcome is inherently noisy but the
//    DISTRIBUTION's moments are known exactly.
//  - fluctua_adjoint() is checked against direct autodiff through the
//    IDENTICAL unrolled Euler-Maruyama discretization (same pattern
//    pluss::ode::fluxa_adjoint's own test uses): the oracle reuses the
//    module's own RNG helper to guarantee it sees the SAME realized
//    noise, but hand-rederives the step formula fresh rather than calling
//    the module's em_step_, so a bug shared between the forward step and
//    the adjoint's own replay of it would not silently cancel out.
#include <pluss/ma.h>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
namespace sde = pluss::subsp::math::sde;
using pluss::subsp::math::diff::trace;

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

void expect_near(double got, double want, double atol, const std::string& msg, int* failures) {
  if (std::abs(got - want) > atol) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << " got=" << got << " want=" << want << " atol=" << atol << '\n';
  }
}

Series real_series(const std::vector<double>& values, const std::vector<std::int64_t>& shape) {
  return pluss::make_series(values, shape).to(ScalarType::Float64);
}

Series constant_series(double value, std::int64_t n) { return real_series(std::vector<double>(n, value), {n}); }

double scalar_f64(const Series& x) { return x.to(pluss::pCPU, ScalarType::Float64).item<double>(); }

std::vector<double> vec_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).reshape({x.numel()}).to_vec<double>();
}

} // namespace

int main() {
  int failures = 0;

  // ---------------------------------------------------------------
  // Ito / Stratonovich: exact discrete identities for f(t, W_t) = W_t.
  // ---------------------------------------------------------------
  {
    const Series like = real_series({0.0}, {1});
    const std::int64_t steps = 50;
    const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, steps, /*seed=*/42);
    sde::WienerIntegrandFn f_w = [](double, const Series& w) { return w; };

    const auto ito = sde::ito_integral(f_w, path);
    const auto strat = sde::stratonovich_integral(f_w, path);

    const double w_T = scalar_f64(path.w.select(0, steps));
    const double qv = scalar_f64(ito.realized_quadratic_variation);

    expect_near(scalar_f64(ito.value), 0.5 * (w_T * w_T - qv), 1e-9,
               "Ito discrete sum matches its exact algebraic identity 0.5*(W_T^2 - QV)", &failures);
    expect_near(scalar_f64(strat.value), 0.5 * w_T * w_T, 1e-9,
               "Stratonovich discrete sum matches its exact algebraic identity 0.5*W_T^2", &failures);
    expect_near(scalar_f64(strat.value) - scalar_f64(ito.value), 0.5 * qv, 1e-9,
               "Stratonovich - Ito equals exactly half the realized quadratic variation", &failures);
  }

  // ---------------------------------------------------------------
  // Ito: E[integral_0^1 W dW] over many independent batched paths is
  // close to its true value 0 (martingale property).
  // ---------------------------------------------------------------
  {
    const std::int64_t n_paths = 20000;
    const Series like = constant_series(0.0, n_paths);
    const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, 100, /*seed=*/7);
    sde::WienerIntegrandFn f_w = [](double, const Series& w) { return w; };
    const auto ito = sde::ito_integral(f_w, path);
    const double mean_val = scalar_f64(ito.value.mean());
    // Ito isometry: Var(integral_0^1 W dW) = integral_0^1 E[W_t^2] dt =
    // integral_0^1 t dt = 0.5, so std error of the mean over n_paths is
    // sqrt(0.5/n_paths) ~ 0.005; 0.05 is a ~10-standard-error bound.
    expect(std::abs(mean_val) < 0.05,
           "E[integral W dW] over 20000 independent batched paths is close to its true value 0", &failures);
  }

  // ---------------------------------------------------------------
  // fluctua(): GBM dY = mu*Y dt + sigma*Y dW, single fixed path, checked
  // against the exact closed form using the SAME realized Wiener path.
  // ---------------------------------------------------------------
  {
    const double y0v = 1.0, muv = 0.08, sigmav = 0.2, t0 = 0.0, t1 = 1.0;
    const Series y0 = real_series({y0v}, {1});
    sde::DriftFn mu = [muv](double, const Series& y, const Series&) { return y.mul(muv); };
    sde::DiffusionFn sigma = [sigmav](double, const Series& y, const Series&) { return y.mul(sigmav); };
    sde::SdeOptions opts;
    opts.steps = 20000;
    opts.seed = 123;
    const auto fwd = sde::fluctua(mu, sigma, y0, t0, t1, pluss::series::undefined(), opts);
    const double w_T = scalar_f64(fwd.wiener_final);
    const double closed_form = y0v * std::exp((muv - 0.5 * sigmav * sigmav) * (t1 - t0) + sigmav * w_T);
    // At 20000 steps this scheme's realized error against the closed form
    // is ~2e-4 (checked directly); 0.005 keeps a real (~20x) safety margin
    // without being so loose it would pass a badly broken implementation.
    expect_near(scalar_f64(fwd.y_final), closed_form, 0.005,
               "fluctua (Euler-Maruyama, 20000 steps) converges to GBM's exact closed form using the SAME "
               "realized Wiener path",
               &failures);
    expect(fwd.converged, "fluctua reports converged", &failures);
  }

  // ---------------------------------------------------------------
  // fluctua(): Ornstein-Uhlenbeck dY = theta*(mu - Y) dt + sigma dW,
  // batched statistical check against the EXACT conditional mean/
  // variance formulas.
  // ---------------------------------------------------------------
  {
    const double speed = 1.5, level = 0.5, sigmav = 0.3, y0v = 0.0, t0 = 0.0, t1 = 0.5;
    const std::int64_t n_paths = 50000;
    const Series y0 = constant_series(y0v, n_paths);
    sde::DriftFn mu = [speed, level](double, const Series& y, const Series&) {
      return y.mul(-1.0).add(level).mul(speed);
    };
    sde::DiffusionFn sigma = [sigmav](double, const Series& y, const Series&) { return y.mul(0.0).add(sigmav); };
    sde::SdeOptions opts;
    opts.steps = 200;
    opts.seed = 99;
    const auto fwd = sde::fluctua(mu, sigma, y0, t0, t1, pluss::series::undefined(), opts);

    const auto vals = vec_f64(fwd.y_final);
    double sum = 0.0, sumsq = 0.0;
    for (double v : vals) {
      sum += v;
      sumsq += v * v;
    }
    const double n = static_cast<double>(vals.size());
    const double emp_mean = sum / n;
    const double emp_var = sumsq / n - emp_mean * emp_mean;

    const double exact_mean = level + (y0v - level) * std::exp(-speed * t1);
    const double exact_var = (sigmav * sigmav) / (2.0 * speed) * (1.0 - std::exp(-2.0 * speed * t1));
    const double se_mean = std::sqrt(exact_var / n);
    const double se_var = exact_var * std::sqrt(2.0 / n);

    // Checked directly: actual deviation is ~0.6-0.7 standard errors at
    // this seed; 5 standard errors keeps this from being flaky under a
    // correct implementation (~1e-6 false-failure rate) while still
    // failing hard if the drift/diffusion wiring were actually wrong.
    expect_near(emp_mean, exact_mean, 5.0 * se_mean + 1e-4,
               "OU empirical mean (50000 batched paths) matches its exact conditional mean formula", &failures);
    expect_near(emp_var, exact_var, 5.0 * se_var + 1e-4,
               "OU empirical variance (50000 batched paths) matches its exact conditional variance formula",
               &failures);
  }

  // ---------------------------------------------------------------
  // fluctua_adjoint(): GBM pathwise sensitivity, checked against direct
  // autodiff through the identical fixed-noise Euler-Maruyama
  // discretization.
  // ---------------------------------------------------------------
  {
    const double y0v = 1.0, muv0 = 0.08, sigmav = 0.2, t0 = 0.0, t1 = 1.0;
    const Series y0 = real_series({y0v}, {1});
    const Series theta0 = real_series({muv0}, {1});
    sde::DriftFn mu = [](double, const Series& y, const Series& theta) { return y.mul(theta); };
    sde::DiffusionFn sigma = [sigmav](double, const Series& y, const Series&) { return y.mul(sigmav); };
    sde::SdeOptions opts;
    opts.steps = 500;
    opts.seed = 55;

    const Series seed_grad = real_series({1.0}, {1});
    const auto adj = sde::fluctua_adjoint(mu, sigma, y0, t0, t1, theta0, seed_grad, opts);

    const double h = (t1 - t0) / static_cast<double>(opts.steps);
    const Series dw = sde::detail::wiener_increments_(y0, opts.steps, h, opts.seed);

    auto ty0 = trace(y0, /*tracks_slope=*/true);
    auto ttheta = trace(theta0, /*tracks_slope=*/true);
    Series y = ty0.value();
    Series theta = ttheta.value();
    for (std::int64_t i = 0; i < opts.steps; ++i) {
      const Series dw_i = dw.select(0, i);
      y = y.add(y.mul(theta).mul(h)).add(y.mul(sigmav).mul(dw_i));
    }
    y.reverse();

    expect_near(scalar_f64(adj.y_final), scalar_f64(y), 1e-9,
               "fluctua_adjoint's forward value matches the independent oracle's forward value", &failures);
    expect_near(scalar_f64(y0.sensitivity()), scalar_f64(adj.grad_y0), 1e-9,
               "fluctua_adjoint grad_y0 matches direct autodiff through the identical fixed-noise discretization",
               &failures);
    expect_near(scalar_f64(theta0.sensitivity()), scalar_f64(adj.grad_theta), 1e-9,
               "fluctua_adjoint grad_theta matches direct autodiff through the identical fixed-noise "
               "discretization",
               &failures);
    expect(adj.forward_steps == opts.steps && adj.backward_steps == opts.steps,
           "fluctua_adjoint reports matching forward/backward step counts", &failures);

    // Determinism: same seed -> same realized path -> same forward value.
    const auto fwd2 = sde::fluctua(mu, sigma, y0, t0, t1, theta0, opts);
    expect_near(scalar_f64(fwd2.y_final), scalar_f64(adj.y_final), 1e-9, "fluctua is deterministic given the same seed",
               &failures);
  }

  // ---------------------------------------------------------------
  // fluctua_adjoint(..., LossFn, ...): auto-derived seed for a nonlinear
  // downstream transform of y_final (Y_T = exp(lambda*X_T)), checked
  // against (a) the SAME manual-seed overload with a hand-computed seed,
  // and (b) an independent central-finite-difference oracle through the
  // whole pipeline (fluctua -> exp(lambda*.)).
  // ---------------------------------------------------------------
  {
    const double x0v = 1.0, thetav = 0.08, sigmav = 0.2, lambdav = 1.5, t0 = 0.0, t1 = 1.0;
    const Series x0 = real_series({x0v}, {1});
    const Series theta0 = real_series({thetav}, {1});
    sde::DriftFn mu = [](double, const Series& y, const Series& theta) { return y.mul(theta); };
    sde::DiffusionFn sigma = [sigmav](double, const Series& y, const Series&) { return y.mul(sigmav); };
    sde::SdeOptions opts;
    opts.steps = 500;
    opts.seed = 77;

    sde::LossFn loss = [lambdav](const Series& y_final) { return y_final.mul(lambdav).exp(); };
    const auto adj_auto = sde::fluctua_adjoint(mu, sigma, x0, t0, t1, theta0, loss, opts);

    // Manual-seed cross-check: the exact same seed, computed by hand.
    const auto fwd = sde::fluctua(mu, sigma, x0, t0, t1, theta0, opts);
    const Series seed = fwd.y_final.mul(lambdav).exp().mul(lambdav);  // d[exp(lambda*y)]/dy = lambda*exp(lambda*y)
    const auto adj_manual = sde::fluctua_adjoint(mu, sigma, x0, t0, t1, theta0, seed, opts);

    expect_near(scalar_f64(adj_auto.grad_theta), scalar_f64(adj_manual.grad_theta), 1e-9,
               "fluctua_adjoint(LossFn) matches the manual-seed overload for the same downstream transform",
               &failures);

    // Independent oracle: central finite difference through the WHOLE
    // pipeline (fluctua -> exp(lambda*.)), same seed, no autodiff at all.
    auto y_of_theta = [&](double th) {
      const auto f = sde::fluctua(mu, sigma, x0, t0, t1, real_series({th}, {1}), opts);
      return scalar_f64(f.y_final.mul(lambdav).exp());
    };
    const double eps = 1e-6;
    const double grad_fd = (y_of_theta(thetav + eps) - y_of_theta(thetav - eps)) / (2 * eps);
    expect_near(scalar_f64(adj_auto.grad_theta), grad_fd, 1e-4,
               "fluctua_adjoint(LossFn)'s auto-derived grad_theta matches an independent finite-difference oracle",
               &failures);
  }

  if (failures != 0) {
    std::cerr << "random_processes_test failed with " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "random_processes_test passed\n";
  return 0;
}
