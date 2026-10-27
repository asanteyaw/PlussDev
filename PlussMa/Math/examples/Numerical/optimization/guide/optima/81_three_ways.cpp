// The same problem, three ways of supplying the derivatives. The objective is the Rosenbrock function on the box
// [-2, 2]^2, started at (-1.2, 1). `autodiff` records the function on the reverse-mode tape and differentiates it for
// you; `analytic` returns the value and the gradient you worked out by hand; `fwd_objective` carries the gradient
// alongside the value in plain doubles. All three describe the same function, so they must agree on the answer and
// on how many iterations it takes. They differ in what you write and what each evaluation costs.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::subsp::math::optim;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> pair(double x, double y) { return optim::pack_scalars({{"x", scalar(x)}, {"y", scalar(y)}}); }

optim::ConstrainedProblem with_box(optim::ProblemBuilder b) { return b.lbs(pair(-2, -2)).ubs(pair(2, 2)).build(); }

int main() {
  // 1. Reverse-mode autodiff: write the function once on pluss::Series values.
  auto traced = with_box(optim::problem().objective([](const optim::ParameterPack<pluss::Series>& p) -> pluss::Series {
    return 100.0 * (p.at("y") - p.at("x").square()).square() + (1.0 - p.at("x")).square();
  }));

  // 2. Analytic: return the value and a pack with the same blocks holding the gradient.
  auto analytic = with_box(optim::problem().objective_analytic([](const optim::ParameterPack<pluss::Series>& p) {
    const double x = as_f64(p.at("x")), y = as_f64(p.at("y"));
    const double value = 100.0 * (y - x * x) * (y - x * x) + (1.0 - x) * (1.0 - x);
    const double gx = -400.0 * x * (y - x * x) - 2.0 * (1.0 - x);
    const double gy = 200.0 * (y - x * x);
    return std::pair{scalar(value), pair(gx, gy)};
  }));

  // 3. Forward mode: the N parameters arrive as an array of Fwd<N> values and the result is one Fwd<N>.
  using F = optim::Fwd<2>;
  auto forward = with_box(optim::problem().objective_analytic(optim::fwd_objective<2>([](const std::array<F, 2>& p) -> F {
    return 100.0 * optim::square(p[1] - optim::square(p[0])) + optim::square(1.0 - p[0]);
  })));

  struct Mode { const char* label; const optim::ConstrainedProblem* problem; };
  const Mode modes[] = {{"autodiff", &traced}, {"analytic", &analytic}, {"fwd_objective", &forward}};
  std::cout << std::left << std::setw(16) << "mode" << std::setw(8) << "exit" << std::setw(12) << "iterations"
            << std::setw(14) << "evaluations" << std::setw(14) << "objective" << "x, y\n";
  for (const auto& m : modes) {
    const auto r = optim::optima(*m.problem, pair(-1.2, 1.0));
    const auto x = as_f64(r.params.at("x"));
    const auto y = as_f64(r.params.at("y"));
    std::cout << std::left << std::setw(16) << m.label << std::setw(8) << static_cast<int>(r.exit_flag) << std::setw(12)
              << r.iteration << std::setw(14) << r.function_evaluations << std::scientific << std::setprecision(2)
              << std::setw(14) << r.evaluation.value.item<double>() << std::fixed << std::setprecision(6) << x << ", " << y
              << "\n";
  }
  return 0;
}
