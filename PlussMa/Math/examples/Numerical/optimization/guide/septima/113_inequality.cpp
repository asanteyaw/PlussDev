// An inequality constraint. The target (2, 1) lies outside the unit disc, and the problem is to be as close to it as
// possible while staying inside: minimize 0.5 * |x - (2, 1)|^2 subject to x0^2 + x1^2 - 1 <= 0. The answer is the point on the
// circle in the direction of the target, (2, 1) / sqrt(5) = (0.8944, 0.4472), and the constraint's multiplier is
// (sqrt(5) - 1) / 2 = 0.618. An inequality residual is positive when the constraint is violated; its multiplier is kept at zero
// or above, grows while the constraint is violated and relaxes while it is slack. The program prints the answer, the violation
// and the multiplier for Adam and for L-BFGS, each from a start inside the disc and from one far outside it, all with the
// library's default constraint schedule. L-BFGS is also run with and without the strong-Wolfe line search.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace optim = pluss::subsp::math::optim;
using pluss::Series;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto target = vec({2.0, 1.0});
  auto objective = [&](const pluss::Series& x) -> pluss::Series { return (x - target).square().sum() * 0.5; };
  optim::SeptimaConstraints disc;
  disc.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].square().sum() - 1.0}; };

  std::cout << "exact answer (" << std::fixed << std::setprecision(4) << 2.0 / std::sqrt(5.0) << ", " << 1.0 / std::sqrt(5.0)
            << "), multiplier " << (std::sqrt(5.0) - 1.0) / 2.0 << "\n\n";
  std::cout << std::left << std::setw(34) << "run" << std::setw(8) << "iters" << std::setw(20) << "x" << std::setw(12) << "violation" << "multiplier\n";
  const auto row = [](const char* label, const optim::SeptimaResult& r) {
    std::ostringstream x;
    x << std::fixed << std::setprecision(4) << r.params[0].select(0, 0).item<double>() << ", " << r.params[0].select(0, 1).item<double>();
    std::cout << std::left << std::setw(34) << label << std::setw(8) << r.iteration << std::setw(20) << x.str() << std::scientific << std::setprecision(1)
              << std::setw(12) << r.constraint_violation << std::fixed << std::setprecision(4) << r.state.lambda_inequalities[0].item<double>() << "\n";
  };

  optim::SeptimaSolveOptions base;  // the library defaults for the constraint schedule
  base.max_iterations = 4000;
  base.gradient_tolerance = 1e-5;
  base.step_tolerance = 1e-5;
  base.step.adam.learning_rate = 0.1;

  row("Adam from (0, 0)", optim::septima(objective, vec({0.0, 0.0}), base, disc));
  row("Adam from (-2, 1.5)", optim::septima(objective, vec({-2.0, 1.5}), base, disc));

  optim::SeptimaSolveOptions lbfgs = base;
  lbfgs.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  row("L-BFGS from (0, 0)", optim::septima(objective, vec({0.0, 0.0}), lbfgs, disc));
  row("L-BFGS from (-2, 1.5)", optim::septima(objective, vec({-2.0, 1.5}), lbfgs, disc));
  lbfgs.step.lbfgs.line_search = optim::SeptimaLbfgsLineSearch::StrongWolfe;
  row("L-BFGS, strong Wolfe, far start", optim::septima(objective, vec({-2.0, 1.5}), lbfgs, disc));
  return 0;
}
