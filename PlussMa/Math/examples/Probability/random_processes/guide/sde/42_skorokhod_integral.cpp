// The Skorokhod integral delta(u) = sum u_i dW_i - sum (du_i / d dW_i) dt extends the Ito integral to
// integrands that look into the future. Two checks. (1) For an adapted integrand it reduces to the Ito sum.
// (2) For the anticipating integrand u_i = W_T (every increment multiplied by the terminal value) it equals
// W_T^2 - T, which no Ito sum can produce, because the Ito sum needs u_i to be known at the start of step i. The
// second check is run on 20000 paths at once, with time on axis 0 and the paths on axis 1.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const std::int64_t n = 50, paths = 20000;
  const double dt = 1.0 / n;
  std::cout << std::fixed << std::setprecision(5);

  // One path: u_i = W_{t_i} (adapted) gives the Ito sum.
  const pluss::Series dw = pluss::series::randn({n}, pluss::Config{}.with_dtype(f64), 8).mul(std::sqrt(dt));
  const auto adapted = [](const pluss::Series& inc) { return inc.cumsum(0).sub(inc); };  // W at the left end of each step
  const double sk = dw.calc.diff.skorokhod(adapted, dt).item<double>();
  const double ito = adapted(dw).mul(dw).sum().item<double>();
  std::cout << "adapted integrand: Skorokhod " << sk << ", Ito sum " << ito << "\n";

  // Anticipating integrand u_i = W_T for every i, over many paths.
  const pluss::Series dws = pluss::series::randn({n, paths}, pluss::Config{}.with_dtype(f64), 9).mul(std::sqrt(dt));
  const auto anticipating = [](const pluss::Series& inc) { return inc.sum(0).unsqueeze(0).expand({inc.size(0), inc.size(1)}); };
  const pluss::Series delta = dws.calc.diff.skorokhod(anticipating, dt);
  const pluss::Series wT = dws.sum(0);
  std::cout << "anticipating u = W_T: mean |delta(u) - (W_T^2 - 1)| = "
            << delta.sub(wT.mul(wT).sub(1.0)).abs().mean().item<double>() << "\n";
  return 0;
}
