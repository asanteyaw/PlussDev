// Three schemes for dX = mu X dt + sigma X dW, compared with the exact solution on the SAME Wiener path (fluctua
// returns the realized W(T), so X_T = x0 exp((mu - sigma^2/2) T + sigma W_T) is available for every path).
// Euler-Maruyama has strong order 1/2; Milstein (adds the sigma sigma' term) and Stratonovich Heun have order 1.
// Heun solves the Stratonovich equation, so it is given the converted drift mu - sigma^2/2 to describe the same
// process. The table shows the mean absolute error of X_T falling as the step count doubles.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace sde = pluss::subsp::math::sde;

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const double mu = 0.2, sg = 0.5, x0 = 1.0, T = 1.0;
  const std::int64_t paths = 20000;
  const pluss::Series y0 = pluss::Series::full({paths}, x0, f64);

  std::cout << std::fixed << std::setprecision(5);
  std::cout << "steps   Euler-Maruyama   Milstein   Heun (Stratonovich form)\n";
  for (std::int64_t steps : {8, 16, 32, 64, 128}) {
    double err[3];
    for (int m = 0; m < 3; ++m) {
      sde::SdeOptions o;
      o.method = m == 0 ? sde::SdeMethod::EulerMaruyama : m == 1 ? sde::SdeMethod::Milstein : sde::SdeMethod::StratonovichHeun;
      o.steps = steps;
      o.seed = 5;  // the same seed gives every scheme the same noise at the same step count
      const double drift = m == 2 ? mu - 0.5 * sg * sg : mu;
      const auto run = sde::fluctua([=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(drift); },
                                    [=](double, const pluss::Series& y, const pluss::Series&) { return y.mul(sg); }, y0, 0.0, T,
                                    pluss::series::undefined(), o);
      const pluss::Series exact = run.wiener_final.mul(sg).add((mu - 0.5 * sg * sg) * T).exp().mul(x0);
      err[m] = run.y_final.sub(exact).abs().mean().item<double>();
    }
    std::cout << std::setw(5) << steps << "   " << std::setw(14) << err[0] << "   " << std::setw(8) << err[1] << "   " << err[2] << "\n";
  }
  return 0;
}
