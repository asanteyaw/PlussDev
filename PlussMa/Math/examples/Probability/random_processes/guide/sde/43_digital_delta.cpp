// A digital option pays 1{X_T > K}. Its pathwise derivative is zero almost everywhere (the payoff is flat except
// at one point), so the pathwise estimator sees nothing. The Malliavin weight does not differentiate the payoff:
// delta = E[ payoff * W_T / (x0 sigma T) ] (undiscounted), valid for GBM, and its closed form is
// n(d2) / (x0 sigma sqrt(T)). The program shows the pathwise estimate (zero) next to the weighted estimate and the
// closed form.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double r = 0.02, sg = 0.3, x0 = 100.0, K = 100.0, T = 1.0;
  const std::int64_t paths = 400000;
  const pluss::Series y0 = pluss::Series::full({paths}, x0, f64);
  sde::SdeOptions o;
  o.steps = 1;  // GBM is solved exactly below from W_T, so one step of the scheme is not used
  o.seed = 77;
  const auto run = sde::fluctua([=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(0.0); },
                                [=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(0.0); }, y0, 0.0, T,
                                pluss::series::undefined(), o);
  const pluss::Series wT = run.wiener_final;  // the realized W_T of every path
  const pluss::Series xT = wT.mul(sg).add((r - 0.5 * sg * sg) * T).exp().mul(x0);
  const pluss::Series payoff = xT.gt(K).to(f64);

  // Pathwise: d payoff / d x0 = (derivative of the indicator) * (d X_T / d x0), and the indicator has slope zero
  // everywhere except at the single point X_T = K, which no path hits. Autodiff therefore has nothing to
  // propagate: a comparison is not a differentiable operation.
  std::cout << std::fixed << std::setprecision(5);
  std::cout << "pathwise estimate: 0 on every path\n";

  const double weighted = payoff.mul(wT).mean().item<double>() / (x0 * sg * T);
  const double d2 = (std::log(x0 / K) + (r - 0.5 * sg * sg) * T) / (sg * std::sqrt(T));
  const double closed = std::exp(-0.5 * d2 * d2) / std::sqrt(2 * M_PI) / (x0 * sg * std::sqrt(T));
  std::cout << "Malliavin-weight estimate  " << weighted << "\n";
  std::cout << "closed form n(d2)/(x0 sigma sqrt(T))  " << closed << "\n";
  return 0;
}
