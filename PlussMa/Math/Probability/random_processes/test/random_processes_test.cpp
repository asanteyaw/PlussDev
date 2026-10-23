// pluss::subsp::math::sde -- Fluctua (fluctua/fluctua_adjoint) and the
// standalone Ito/Stratonovich stochastic-integral primitives.
//
// Verification strategy, mirroring this program's established discipline:
//  - ito_integral()/strat_integral() are checked against EXACT
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
    const auto strat = sde::strat_integral(f_w, path);

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

  // ---- path diagnostics --------------------------------------------------
  {
    const Series like = real_series({0.0, 0.0, 0.0}, {3});
    const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, 400, /*seed=*/5);
    const auto r = sde::ito_integral([](double, const Series& w) { return w; }, path);
    expect(r.warnings.empty(), "a standard Wiener path raises no warning", &failures);
    expect_near(r.quadratic_variation_ratio, 1.0, 0.2, "the quadratic variation ratio of a Wiener path is about 1",
               &failures);
    sde::WienerPath scaled = path;
    scaled.w = path.w.mul(3.0);
    const auto s = sde::ito_integral([](double, const Series& w) { return w; }, scaled);
    expect(!s.warnings.empty(), "a path with the wrong volatility is flagged", &failures);
    expect_near(s.quadratic_variation_ratio, 9.0, 1.5, "the ratio reflects the volatility (9 for a tripled path)", &failures);
  }

  // ---- adaptedness probe for integrands computed from the whole path ----------
  {
    const Series like = real_series({0.0, 0.0}, {2});
    const auto path = sde::simulate_wiener_path(like, 0.0, 1.0, 120, /*seed=*/9);
    // adapted: the value of the path, and a running maximum
    auto identity = [](const Series& w) { return w; };
    auto running_max = [](const Series& w) {
      std::vector<Series> rows;
      Series best = w.select(0, 0);
      rows.push_back(best.unsqueeze(0));
      for (std::int64_t i = 1; i < w.size(0); ++i) {
        best = best.maximum(w.select(0, i));
        rows.push_back(best.unsqueeze(0));
      }
      Series out = rows[0];
      for (std::size_t i = 1; i < rows.size(); ++i) out = out.unite({rows[i]}, 0);
      return out;
    };
    const auto a = sde::ito_integral(sde::SampledIntegrandFn(identity), path);
    const auto b = sde::ito_integral([](double, const Series& w) { return w; }, path);
    const auto vec_a = vec_f64(a.value), vec_b = vec_f64(b.value);
    for (std::size_t i = 0; i < vec_a.size(); ++i)
      expect_near(vec_a[i], vec_b[i], 1e-12, "a sampled integrand gives the same Ito sum as the pointwise form", &failures);
    const auto sa = sde::strat_integral(sde::SampledIntegrandFn(identity), path);
    const auto sb = sde::strat_integral([](double, const Series& w) { return w; }, path);
    expect_near(vec_f64(sa.value)[0], vec_f64(sb.value)[0], 1e-12, "the sampled Stratonovich sum matches", &failures);
    bool ok = true;
    try { (void)sde::ito_integral(sde::SampledIntegrandFn(running_max), path); } catch (...) { ok = false; }
    expect(ok, "a running maximum (adapted) passes the probe", &failures);

    // anticipating: a centered moving average uses the next sample; the terminal value uses all of them
    auto centered = [](const Series& w) {
      const std::int64_t n = w.size(0);
      const Series prev = w.slice(0, 0, 1, 1).unite({w.slice(0, 0, n - 1, 1)}, 0);
      const Series next = w.slice(0, 1, n, 1).unite({w.slice(0, n - 1, n, 1)}, 0);
      return prev.add(w).add(next).div(3.0);
    };
    auto terminal = [](const Series& w) {
      return w.select(0, w.size(0) - 1).unsqueeze(0).expand({w.size(0), w.size(1)});
    };
    for (auto* name : {"centered moving average", "terminal value"}) {
      bool caught = false;
      std::string message;
      try {
        (void)sde::ito_integral(std::string(name) == "terminal value" ? sde::SampledIntegrandFn(terminal)
                                                                      : sde::SampledIntegrandFn(centered), path);
      } catch (const std::exception& e) { caught = true; message = e.what(); }
      expect(caught, std::string(name) + " is rejected as anticipating", &failures);
      expect(message.find("not adapted") != std::string::npos, std::string(name) + ": the message says why", &failures);
    }
  }

  // ---- Marcus integral on a jump-diffusion path --------------------------------
  {
    const Series like = real_series({0.0, 0.0, 0.0}, {3});
    const auto jp = sde::simulate_jump_diffusion_path(like, 0.0, 1.0, 500, /*seed=*/17, /*drift=*/0.1,
                                                      /*volatility=*/0.4, /*jump_rate=*/12.0, /*jump_mean=*/0.4,
                                                      /*jump_stddev=*/0.1);
    expect(jp.jump_mask.sum().item<double>() > 3.0, "the jump path contains jumps", &failures);
    const auto marcus = sde::marcus_integral([](double, const Series& x) { return x; }, jp.path, jp.jump_mask);
    const auto ito = sde::ito_integral([](double, const Series& x) { return x; }, jp.path);
    const Series xt = jp.path.w.select(0, 500);
    const auto m = vec_f64(marcus.value), i = vec_f64(ito.value), x = vec_f64(xt);
    const auto q = vec_f64(marcus.realized_quadratic_variation);
    for (std::size_t k = 0; k < 3; ++k) {
      expect_near(m[k], 0.5 * x[k] * x[k], 1e-9, "Marcus integral of X dX is X_T^2 / 2 along a jump path (Newton-Leibniz)", &failures);
      expect_near(i[k], 0.5 * (x[k] * x[k] - q[k]), 1e-9, "the Ito sum pays the full quadratic variation, jumps included", &failures);
    }
    const auto auto_flagged = sde::marcus_integral([](double, const Series& xx) { return xx; }, jp.path, 5.0);
    expect(auto_flagged.warnings.size() <= 1, "automatic jump detection runs", &failures);
    bool too_coarse = false;
    try { (void)sde::simulate_jump_diffusion_path(like, 0.0, 1.0, 10, 1, 0.0, 0.1, 5.0, 0.0, 0.1); } catch (...) { too_coarse = true; }
    expect(too_coarse, "a step too coarse for the jump rate is rejected", &failures);
  }

  // ---- Milstein and Heun: strong convergence orders --------------------------------
  {
    const double mu_c = 0.2, sg = 0.8, T = 1.0;
    const std::int64_t paths = 6000;
    const Series y0 = constant_series(1.0, paths);
    auto mu_ito = [&](double, const Series& y, const Series&) { return y.mul(mu_c); };
    auto sigma_f = [&](double, const Series& y, const Series&) { return y.mul(sg); };
    auto mu_strat = [&](double, const Series& y, const Series&) { return y.mul(mu_c - 0.5 * sg * sg); };
    auto strong_error = [&](sde::SdeMethod method, std::int64_t steps, bool strat) {
      sde::SdeOptions o;
      o.method = method; o.steps = steps; o.seed = 77;
      const auto r = sde::fluctua(strat ? sde::DriftFn(mu_strat) : sde::DriftFn(mu_ito), sigma_f, y0, 0.0, T,
                                  pluss::series::undefined(), o);
      // Ito GBM: X_T = exp((mu - s^2/2) T + s W_T).  The Stratonovich SDE with drift (mu - s^2/2) has the same law
      // path by path: X_T = exp(mu_s T + s W_T + ...) -- both reduce to the same closed form here.
      const Series exact = r.wiener_final.mul(sg).add((mu_c - 0.5 * sg * sg) * T).exp();
      return scalar_f64(r.y_final.sub(exact).abs().mean());
    };
    const double em_coarse = strong_error(sde::SdeMethod::EulerMaruyama, 20, false);
    const double em_fine = strong_error(sde::SdeMethod::EulerMaruyama, 80, false);
    const double mi_coarse = strong_error(sde::SdeMethod::Milstein, 20, false);
    const double mi_fine = strong_error(sde::SdeMethod::Milstein, 80, false);
    const double he_coarse = strong_error(sde::SdeMethod::StratonovichHeun, 20, true);
    const double he_fine = strong_error(sde::SdeMethod::StratonovichHeun, 80, true);
    expect(em_coarse / em_fine > 1.5 && em_coarse / em_fine < 2.8, "Euler-Maruyama halves its error when the step is cut 4x (order 1/2)", &failures);
    expect(mi_coarse / mi_fine > 3.0 && mi_coarse / mi_fine < 5.5, "Milstein cuts its error ~4x when the step is cut 4x (order 1)", &failures);
    expect(he_coarse / he_fine > 3.0 && he_coarse / he_fine < 5.5, "the Stratonovich Heun scheme is of order 1 as well", &failures);
    expect(mi_fine < em_fine, "Milstein beats Euler-Maruyama at the same step", &failures);
  }

  // ---- pathwise adjoint through Milstein and Heun, against finite differences -----------
  for (auto method : {sde::SdeMethod::Milstein, sde::SdeMethod::StratonovichHeun}) {
    const Series y0 = real_series({1.0, 1.4}, {2});
    auto mu = [](double, const Series& y, const Series& th) { return y.mul(th.select(0, 0)); };
    auto sigma = [](double, const Series& y, const Series& th) { return y.mul(th.select(0, 1)).add(y.mul(y).mul(0.05)); };
    const sde::LossFn loss = [](const Series& y) { return y.mul(y).sum(); };
    sde::SdeOptions o;
    o.method = method; o.steps = 40; o.seed = 4;
    const Series theta = real_series({0.3, 0.5}, {2});
    const auto adj = sde::fluctua_adjoint(mu, sigma, y0, 0.0, 1.0, theta, loss, o);
    for (std::int64_t k = 0; k < 2; ++k) {
      auto value = [&](double shift) {
        std::vector<double> th = {0.3, 0.5};
        th[static_cast<std::size_t>(k)] += shift;
        const auto f = sde::fluctua(mu, sigma, y0, 0.0, 1.0, real_series(th, {2}), o);
        return scalar_f64(f.y_final.mul(f.y_final).sum());
      };
      const double eps = 1e-6;
      const double fd = (value(eps) - value(-eps)) / (2 * eps);
      expect_near(vec_f64(adj.grad_theta)[static_cast<std::size_t>(k)], fd, 1e-4 * (1.0 + std::abs(fd)),
                 std::string("adjoint gradient matches finite differences for ") +
                     (method == sde::SdeMethod::Milstein ? "Milstein" : "Heun") + ", parameter " + std::to_string(k),
                 &failures);
    }
    const auto fwd = sde::fluctua(mu, sigma, y0, 0.0, 1.0, theta, o);
    expect_near(vec_f64(adj.y_final)[0], vec_f64(fwd.y_final)[0], 1e-12, "adjoint forward pass equals fluctua", &failures);
  }

  if (failures != 0) {
    std::cerr << "random_processes_test failed with " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "random_processes_test passed\n";
  return 0;
}
