#include <pluss/pluss.h>
#include <pluss/ma.h>
#include <pluss/subsp/math/diff.h>

#include <iostream>
#include <vector>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;

  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  Series x = pluss::make_series(std::vector<double>{0.5, -0.5}, {2}, cfg);
  const Series target = pluss::make_series(std::vector<double>{2.0, 1.0}, {2}, cfg);

  pluss::subsp::math::optim::SeptimaSolveOptions options{};
  options.max_iterations = 120;
  options.gradient_tolerance = 1e-8;
  options.constraint_tolerance = 1e-8;
  options.step.adam.learning_rate = 0.04;
  options.step.dual_learning_rate = 0.08;
  options.step.equality_penalty = 6.0;
  options.step.inequality_penalty = 6.0;
  options.step.project_bounds = true;
  options.step.enable_constraint_autodiff = true;

  pluss::subsp::math::optim::SeptimaConstraints constraints{};
  constraints.equalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].sum() - 3.0};
  };
  constraints.inequalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].square().sum() - 10.0};
  };
  constraints.lbs = {pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg)};

  options.progress_hook = [&](const pluss::subsp::math::optim::SeptimaProgressSnapshot& s) {
    if (s.iteration % 30 == 0 || s.iteration == 1) {
      std::cout << "iter=" << s.iteration
                << " obj=" << s.objective
                << " viol=" << s.constraint_violation
                << " grad_inf=" << s.gradient_inf_norm
                << " step_inf=" << s.step_inf_norm << '\n';
    }
    return false;
  };

  const auto fit = pluss::subsp::math::optim::septima(
      [&](const Series& v) -> Series {
        return ((v - target).square()).sum() *
               0.5;
      },
      x,
      options,
      constraints);

  std::cout << "final: [" << fit.params[0].select(0, 0).item<double>()
            << ", " << fit.params[0].select(0, 1).item<double>() << "]\n";

  return 0;
}
