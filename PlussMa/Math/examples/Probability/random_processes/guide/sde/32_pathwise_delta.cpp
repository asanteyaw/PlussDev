// The pathwise adjoint differentiates a payoff through the simulated paths. For a call, d payoff / d x0 =
// 1{X_T > K} X_T / x0 on each path, and the mean of that is the Black-Scholes delta N(d1) (discounting aside).
// fluctua_adjoint runs the forward solve, stores the realized noise, and sweeps backward, returning d loss / d y0
// for every path; the loss is any scalar function of y_final.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double r = 0.03, sg = 0.25, x0 = 100.0, K = 105.0, T = 1.0;
  const std::int64_t paths = 100000;
  const pluss::Series y0 = pluss::Series::full({paths}, x0, f64);
  sde::SdeOptions o;
  o.method = sde::SdeMethod::Milstein;
  o.steps = 64;
  o.seed = 21;

  const auto adj = sde::fluctua_adjoint(
      [=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(r); },
      [=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(sg); }, y0, 0.0, T, pluss::series::undefined(),
      [=](const pluss::Series& yT) { return yT.sub(K).relu().sum().mul(1.0 / static_cast<double>(paths)); }, o);

  const double delta_mc = adj.grad_y0.sum().item<double>();  // loss was the mean, so summing the per-path gradients gives the mean derivative
  const double d1 = (std::log(x0 / K) + (r + 0.5 * sg * sg) * T) / (sg * std::sqrt(T));
  const double delta_bs = std::exp(r * T) * 0.5 * std::erfc(-d1 / std::sqrt(2.0));
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "pathwise delta (adjoint)      " << delta_mc << "\n";
  std::cout << "closed form e^(rT) N(d1)      " << delta_bs << "\n";
  const pluss::Series per_path = adj.grad_y0.mul(static_cast<double>(paths));  // the loss was a mean: undo the 1/N
  const double se = std::sqrt(per_path.sub(delta_mc).pow(2.0).mean().item<double>() / static_cast<double>(paths));
  std::cout << "standard error of the estimate  " << se << "\n";
  return 0;
}
