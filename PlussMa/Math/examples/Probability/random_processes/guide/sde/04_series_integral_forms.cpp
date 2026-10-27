// The integrals are methods of the path itself: `path.calc.antidiff.ito(f, t0, t1)` integrates f(t, W_t) dW along
// the path stored in the receiver. The integrand is vectorized over the time axis: it is called once with the
// time of every grid point and the path value there, and must act pointwise, so it cannot look at any other
// sample. This program shows the pieces: a time-dependent integrand, an explicit non-uniform time grid, a path
// whose time axis is the last one, and the running integral.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  const std::int64_t steps = 500;
  const pluss::Series dw = pluss::series::randn({steps, 2}, pluss::Config{}.with_dtype(f64), 3).mul(std::sqrt(1.0 / steps));
  const pluss::Series w = pluss::Series::zeros({1, 2}, f64).unite({dw.cumsum(0)}, 0);  // [steps + 1, 2]

  // f(t, w) = t * w: the integrand depends on time and on the path value.
  auto f = [](const pluss::Series& t, const pluss::Series& x) { return x.mul(t); };
  std::cout << std::fixed << std::setprecision(5);
  const auto total = w.calc.antidiff.ito(f, 0.0, 1.0).host<double>();
  std::cout << "uniform grid, Ito:          " << total[0] << "  " << total[1] << "\n";
  const auto strat = w.calc.antidiff.strat(f, 0.0, 1.0).host<double>();
  std::cout << "uniform grid, Stratonovich: " << strat[0] << "  " << strat[1] << "\n";

  // An explicit grid: the same samples, read as if time ran faster at the end. The increments of the path are
  // unchanged, only the times fed to f differ.
  const pluss::Series grid = pluss::Series::linspace(0.0, 1.0, steps + 1, f64).pow(2.0);
  const auto warped = w.calc.antidiff.ito(f, grid).host<double>();
  std::cout << "warped grid, Ito:           " << warped[0] << "  " << warped[1] << "\n";

  // The same path stored with time on the last axis: pass dim = 1.
  const pluss::Series wt = w.transpose(0, 1).contiguous();  // [2, steps + 1]
  const auto transposed = wt.calc.antidiff.ito(f, pluss::Series::linspace(0.0, 1.0, steps + 1, f64), 1).host<double>();
  std::cout << "time on the last axis:      " << transposed[0] << "  " << transposed[1] << "\n";

  // The running integral has the shape of the path and starts at zero.
  const pluss::Series running = w.calc.antidiff.ito_cumulative(f, 0.0, 1.0);
  std::cout << "\nrunning integral: shape [" << running.size(0) << ", " << running.size(1) << "], first row "
            << running.select(0, 0).host<double>()[0] << ", last row " << running.select(0, steps).host<double>()[0]
            << " (equals the total above)\n";
  return 0;
}
