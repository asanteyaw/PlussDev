<!-- generated from yield_curve_filtering.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Filtering the short rate from bond prices

[Back to Filtering](../finance_09_filtering.md)

The short rate is never observed; bond prices are. A filter turns a stream of bond prices into a stream of estimates of the rate, using the rate model to know how the rate can move between observations and the bond formulas to know what each price says about it. This page does that for the Vasicek model of Chapter 7, with three zero-coupon bonds, and runs the same problem through the three Gaussian filters of the library.

## The model

Over a step {math}`\Delta`, the Vasicek rate is an autoregression around its level, with the exact step

```{math}
r_t=\theta+F\,(r_{t-1}-\theta)+w_t,\qquad F=e^{-\kappa\Delta},\qquad w_t\sim\mathcal N\Big(0,\;Q=\frac{\sigma^2(1-F^2)}{2\kappa}\Big).
```

A zero-coupon bond of maturity {math}`\tau` has the price {math}`P=A(\tau)e^{-B(\tau)r}` (Chapter 7), observed with a relative error {math}`\varepsilon`:

```{math}
P_{t,i}=A(\tau_i)\,e^{-B(\tau_i)r_t}\,(1+\varepsilon_{t,i}),\qquad \ln P_{t,i}\approx\ln A(\tau_i)-B(\tau_i)\,r_t+\varepsilon_{t,i}.
```

Two descriptions of the observation follow. The price is a **nonlinear** function of the state, which the extended and the unscented filter handle directly. The logarithm of the price is **linear** in the state, {math}`\ln P-\ln A=-B\,r`, so the Kalman filter applies to the log prices exactly, with {math}`H_i=-B(\tau_i)`. The transition has an intercept, {math}`\theta(1-F)`, which is a constant input.

## The nonlinear filters

For a nonlinear model {math}`x_t=f(x_{t-1})+w_t`, {math}`y_t=h(x_t)+v_t` the library has two filters that keep the Gaussian form of the Kalman filter.

```cpp
filtering::KalmanResult filtering::ekf_filter(const Series &observations, const filtering::EkfModel &model, Config cfg = {});
filtering::KalmanResult filtering::ukf_filter(const Series &observations, const filtering::UkfModel &model, Config cfg = {});
```

The **extended Kalman filter** (EKF) linearises the model around the current estimate: it runs the Kalman equations with {math}`F` and {math}`H` replaced by the Jacobians of {math}`f` and {math}`h`. It needs those Jacobians from you. The **unscented Kalman filter** (UKF) needs no Jacobian: it passes a small set of **sigma points**, chosen to have the mean and covariance of the current estimate, through the model, and rebuilds the mean and covariance from the results. Both return the same `KalmanResult` as the Kalman filter.

| `EkfModel` member | Meaning |
|---|---|
| `transition`, `observation` | the functions {math}`f` and {math}`h`: a state `Series` of shape `[n]` and the date index in, a `Series` of shape `[n]` and `[m]` out |
| `transition_jacobian`, `observation_jacobian` | their Jacobians, of shape `[n, n]` and `[m, n]` |
| `process_cov`, `observation_cov`, `initial_mean`, `initial_cov` | as for the Kalman filter |
| `info_tol` | as for the Kalman filter |

| `UkfModel` member | Meaning |
|---|---|
| `transition`, `observation` | as for the EKF; no Jacobians |
| `process_cov`, `observation_cov`, `initial_mean`, `initial_cov`, `info_tol` | as for the Kalman filter |
| `options.alpha`, `options.beta`, `options.kappa` | the spread of the sigma points around the mean (default 10⁻³), the prior on the distribution (2 is right for a Gaussian) and a secondary spread (0) |
| `options.jitter` | a small amount added to the diagonal of a covariance so that its square root exists (10⁻¹⁰) |

## Program 1: three filters, one problem

The program simulates 20 years of monthly short rates from a Vasicek model, with the long-run level 3%, and observes three bonds (1, 5 and 10 years) with a 0.07% relative error. It then estimates the rate with the Kalman filter on the log prices, and the EKF and the UKF on the prices, and prints the root-mean-square error of each. For comparison it also inverts the one-year bond alone for the rate, with no dynamics.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
namespace rt = fin::rates;
using namespace pluss::tils;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::VasicekParams p{.kappa = 0.8, .theta = 0.03, .sigma = 0.012, .r0 = 0.03};
  const double dt = 1.0 / 12, noise = 0.0007;
  const std::int64_t T = 240;
  const std::vector<double> maturities{1.0, 5.0, 10.0};

  // the hidden short rate, monthly for 20 years, and three bond prices observed with 0.07% noise
  rt::SimOptions options;
  options.scheme = rt::Scheme::Transition;
  options.seed = 3;
  const auto truth = rt::vasicek_paths(T, 1, dt, p, options, as_f64()).rate.slice(0, 1, T + 1).reshape({T});
  const auto r = truth.items<double>();
  const auto B = [&](double tau) { return (1.0 - std::exp(-p.kappa * tau)) / p.kappa; };
  const auto lnA = [&](double tau) { return (p.theta - p.sigma * p.sigma / (2.0 * p.kappa * p.kappa)) * (B(tau) - tau) - p.sigma * p.sigma * B(tau) * B(tau) / (4.0 * p.kappa); };
  const auto eps = (pluss::series::randn({T * 3}, as_f64(), 9) * noise).items<double>();
  std::vector<double> price(T * 3);
  for (std::int64_t t = 0; t < T; ++t) {
    for (int k = 0; k < 3; ++k) price[t * 3 + k] = std::exp(lnA(maturities[k]) - B(maturities[k]) * r[t]) * (1.0 + eps[t * 3 + k]);
  }
  const auto prices = pluss::make_series(price, {T, 3}, as_f64());

  // the rate is AR(1) around theta with the exact Vasicek step: r_t = F r_{t-1} + theta (1 - F) + noise
  const double F = std::exp(-p.kappa * dt), q = p.sigma * p.sigma * (1.0 - F * F) / (2.0 * p.kappa), stationary = p.sigma * p.sigma / (2.0 * p.kappa);
  // linear observation: ln P - ln A = -B r + noise, so the Kalman filter applies to the log prices
  std::vector<double> H, shift;
  for (const double m : maturities) {
    H.push_back(-B(m));
    shift.push_back(lnA(m));
  }
  flt::KalmanModel kalman;
  kalman.transition = pluss::series::full({1, 1}, F, as_f64());
  kalman.observation = pluss::make_series(H, {3, 1}, as_f64());
  kalman.process_cov = pluss::series::full({1, 1}, q, as_f64());
  kalman.observation_cov = pluss::series::eye(3, as_f64()) * (noise * noise);
  kalman.initial_mean = pluss::series::full({1}, p.theta, as_f64());
  kalman.initial_cov = pluss::series::full({1, 1}, stationary, as_f64());
  kalman.control = pluss::series::ones({T, 1}, as_f64());
  kalman.control_matrix = pluss::series::full({1, 1}, p.theta * (1.0 - F), as_f64());
  const auto kalman_result = flt::kalman_filter(prices.log() - pluss::make_series(shift, {1, 3}, as_f64()), kalman, as_f64());

  // the same problem with the prices themselves: a nonlinear observation, P = A exp(-B r)
  const auto observation = [&](const Series &x, std::int64_t) {
    std::vector<Series> rows;
    for (const double m : maturities) rows.push_back((x.reshape({1}) * (-B(m))).exp() * std::exp(lnA(m)));
    return pluss::series::pile(pluss::sign_list(rows), 0).reshape({3});
  };
  const auto jacobian = [&](const Series &x, std::int64_t) {
    std::vector<Series> rows;
    for (const double m : maturities) rows.push_back((x.reshape({1}) * (-B(m))).exp() * (-B(m) * std::exp(lnA(m))));
    return pluss::series::pile(pluss::sign_list(rows), 0).reshape({3, 1});
  };
  const auto transition = [&](const Series &x, std::int64_t) { return x * F + p.theta * (1.0 - F); };
  const Series price_noise = pluss::series::eye(3, as_f64()) * std::pow(0.8 * noise, 2.0);  // prices are about 0.8, the noise is relative

  flt::EkfModel ekf;
  ekf.transition = transition;
  ekf.observation = observation;
  ekf.transition_jacobian = [&](const Series &, std::int64_t) { return pluss::series::full({1, 1}, F, as_f64()); };
  ekf.observation_jacobian = jacobian;
  ekf.process_cov = pluss::series::full({1, 1}, q, as_f64());
  ekf.observation_cov = price_noise;
  ekf.initial_mean = pluss::series::full({1}, p.theta, as_f64());
  ekf.initial_cov = pluss::series::full({1, 1}, stationary, as_f64());
  const auto ekf_result = flt::ekf_filter(prices, ekf, as_f64());

  flt::UkfModel ukf;
  ukf.transition = transition;
  ukf.observation = observation;
  ukf.process_cov = ekf.process_cov;
  ukf.observation_cov = price_noise;
  ukf.initial_mean = ekf.initial_mean;
  ukf.initial_cov = ekf.initial_cov;
  const auto ukf_result = flt::ukf_filter(prices, ukf, as_f64());

  const auto rmse = [&](const Series &estimate) { return (estimate.reshape({T}) - truth).square().mean().sqrt().item<double>(); };
  out.writeln("rmse of the short rate (basis points = 1e-4):");
  out.writeln("  Kalman filter on log prices ", rmse(kalman_result.filtered_mean), " log-likelihood", kalman_result.loglik);
  out.writeln("  extended Kalman filter      ", rmse(ekf_result.filtered_mean), " log-likelihood", ekf_result.loglik);
  out.writeln("  unscented Kalman filter     ", rmse(ukf_result.filtered_mean), " log-likelihood", ukf_result.loglik);
  const auto one_year = (-(prices.select(1, 0).log() - lnA(1.0)) / B(1.0));  // invert the one-year price for the rate, ignoring the dynamics
  out.writeln("  one-year bond inverted alone", rmse(one_year));
  out.writeln("  the first dates: true rate", truth.slice(0, 0, 4).items<double>(), " Kalman", kalman_result.filtered_mean.select(1, 0).slice(0, 0, 4).items<double>());
}
```

Output

```text
rmse of the short rate (basis points = 1e-4):
  Kalman filter on log prices  0.000351  log-likelihood 3672.956978
  extended Kalman filter       0.000356  log-likelihood 3778.703948
  unscented Kalman filter      0.000356  log-likelihood 3778.780857
  one-year bond inverted alone 0.001005
  the first dates: true rate [0.025929, 0.021982, 0.016521, 0.022787]  Kalman [0.026059, 0.021690, 0.016384, 0.022649]
```

The three filters recover the short rate to 3.5 to 3.6 basis points: 0.000351 for the Kalman filter on the log prices, 0.000356 for the EKF and 0.000356 for the UKF. The model is so close to linear over the range of the rate that they agree. Taking the one-year bond on its own and inverting it, with no dynamics and no other bond, gives 0.001005, three times worse: the filter is better because it combines three bonds and the knowledge of how the rate moves. The first dates show the Kalman estimates within 0.0003 of the truth.

The log-likelihoods are not comparable across the two descriptions: 3672.96 is the likelihood of the log prices, and 3778.7 that of the prices; they differ by the Jacobian of the logarithm, {math}`-\sum\ln P_{t,i}=105.7` over the 720 observations. Within one description the likelihood compares models, and the EKF and UKF values, 3778.70 and 3778.78, are the same likelihood.

## Program 2: the settings of the unscented filter

The sigma points are placed at a distance of {math}`\alpha\sqrt{n+\kappa}` standard deviations from the mean. A small {math}`\alpha` keeps them close, which is accurate when the model is nearly linear; a larger one reaches further into the nonlinearity. The program filters the five-year bond alone with a range of settings, and then with an initial mean that is wrong by 7 points and with observation noise that is misjudged by a factor of 5 in each direction.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace fin = pluss::subsp::math::finance;
namespace flt = fin::filtering;
namespace rt = fin::rates;
using namespace pluss::tils;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::VasicekParams p{.kappa = 0.8, .theta = 0.03, .sigma = 0.012, .r0 = 0.03};
  const double dt = 1.0 / 12, noise = 0.0007, B5 = (1.0 - std::exp(-p.kappa * 5.0)) / p.kappa;
  const std::int64_t T = 240;
  rt::SimOptions options;
  options.scheme = rt::Scheme::Transition;
  options.seed = 3;
  const auto truth = rt::vasicek_paths(T, 1, dt, p, options, as_f64()).rate.slice(0, 1, T + 1).reshape({T});
  const double lnA5 = (p.theta - p.sigma * p.sigma / (2.0 * p.kappa * p.kappa)) * (B5 - 5.0) - p.sigma * p.sigma * B5 * B5 / (4.0 * p.kappa);
  // one bond only, the 5-year price
  const auto prices = ((truth * (-B5)).exp() * std::exp(lnA5)) * (pluss::series::randn({T}, as_f64(), 9) * noise + 1.0);
  const double F = std::exp(-p.kappa * dt), q = p.sigma * p.sigma * (1.0 - F * F) / (2.0 * p.kappa), stationary = p.sigma * p.sigma / (2.0 * p.kappa);

  const auto run = [&](flt::UkfOptions options, double initial_mean, double observation_noise) {
    flt::UkfModel model;
    model.transition = [&](const Series &x, std::int64_t) { return x * F + p.theta * (1.0 - F); };
    model.observation = [&](const Series &x, std::int64_t) { return (x.reshape({1}) * (-B5)).exp() * std::exp(lnA5); };
    model.process_cov = pluss::series::full({1, 1}, q, as_f64());
    model.observation_cov = pluss::series::full({1, 1}, observation_noise * observation_noise, as_f64());
    model.initial_mean = pluss::series::full({1}, initial_mean, as_f64());
    model.initial_cov = pluss::series::full({1, 1}, stationary, as_f64());
    model.options = options;
    const auto result = flt::ukf_filter(prices, model, as_f64());
    return std::pair{(result.filtered_mean.reshape({T}) - truth).square().mean().sqrt().item<double>(), result.loglik};
  };
  out.writeln("settings                                  rmse of the rate   log-likelihood");
  const auto show = [&](const std::string &label, std::pair<double, double> r) { out.writeln(label, r.first, r.second); };
  const flt::UkfOptions defaults;
  show("defaults (alpha 1e-3, beta 2, kappa 0)       ", run(defaults, p.theta, 0.8 * noise));
  for (const double alpha : {0.1, 0.5, 1.0}) {
    flt::UkfOptions o;
    o.alpha = alpha;
    show("alpha " + std::to_string(alpha).substr(0, 3) + "                                    ", run(o, p.theta, 0.8 * noise));
  }
  flt::UkfOptions spread;
  spread.alpha = 1.0;
  spread.kappa = 2.0;
  show("alpha 1, kappa 2                           ", run(spread, p.theta, 0.8 * noise));
  show("initial mean 0.10, far from the level      ", run(defaults, 0.10, 0.8 * noise));
  show("observation noise 5 times too small        ", run(defaults, p.theta, 0.8 * noise / 5.0));
  show("observation noise 5 times too large        ", run(defaults, p.theta, 0.8 * noise * 5.0));
}
```

Output

```text
settings                                  rmse of the rate   log-likelihood
defaults (alpha 1e-3, beta 2, kappa 0)        0.000551 1020.736643
alpha 0.1                                     0.000551 1020.736642
alpha 0.5                                     0.000551 1020.736617
alpha 1.0                                     0.000551 1020.736537
alpha 1, kappa 2                            0.000551 1020.736112
initial mean 0.10, far from the level       0.000578 992.337351
observation noise 5 times too small         0.000554 1021.064269
observation noise 5 times too large         0.001133 989.971874
```

With this nearly linear model the sigma-point settings make no difference: from {math}`\alpha=10^{-3}` to 1, and with {math}`\kappa=2`, the error is 0.000551 and the log-likelihood 1020.74 to the digits shown. The robustness checks: an initial mean 7 points from the truth costs 0.000578 instead of 0.000551, since after a few months the observations dominate; observation noise taken as 5 times too small barely changes the estimate (0.000554), but the filter then trusts the data too much and a jump in the noise would pass into the estimate; taking it 5 times too large doubles the error to 0.001133, because the filter discounts the data and follows the dynamics, which are slow. The log-likelihoods of runs with different noise assumptions cannot be compared, since each is the likelihood under its own assumption.

Two properties are worth knowing. On a linear model with Gaussian noise the EKF and the UKF both reduce to the Kalman filter, to the digits of the computation, for every {math}`\alpha`, and this is a useful test of any model that you write. And for a strongly nonlinear observation, such as a square, whose posterior has two branches, {math}`\pm x`, that no Gaussian describes, these filters can fail; the next page's particle filter has no such restriction.

## Checklist

1. If the observation is linear in a transformation of the data (the log of a price), use the Kalman filter on that.
2. Otherwise write the Jacobians for the EKF, or use the UKF and write none.
3. Check a new model against the Kalman filter in the linear case; the EKF and the UKF must reproduce it.
4. Express the noise in the units of the observation. Do not compare the likelihoods of runs with different noise assumptions.
5. Keep the observation noise realistic: too small trusts the data, too large ignores it.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="the_kalman_filter.html">
    <span class="label">Previous</span>
    <span class="title">The Kalman filter</span>
  </a>
  <a class="next-link" href="particle_filters.html">
    <span class="label">Next</span>
    <span class="title">Particle filters</span>
  </a>
</div>
<!-- apiref:nav:end -->
