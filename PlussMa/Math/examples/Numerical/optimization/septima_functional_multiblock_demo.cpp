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

  Series w = pluss::make_series(std::vector<double>{0.25, 0.25}, {2}, cfg);
  Series b = pluss::make_series(std::vector<double>{-1.0}, {1}, cfg);

  const Series w_target = pluss::make_series(std::vector<double>{2.0, 1.0}, {2}, cfg);
  const Series b_target = pluss::make_series(std::vector<double>{0.5}, {1}, cfg);

  pluss::subsp::math::optim::SeptimaSolveOptions options{};
  options.max_iterations = 200;
  options.gradient_tolerance = 1e-8;
  options.constraint_tolerance = 1e-8;
  options.step.adam.learning_rate = 0.03;
  options.step.adam.weight_decay = 1e-3;
  options.step.dual_learning_rate = 0.08;
  options.step.equality_penalty = 8.0;
  options.step.inequality_penalty = 8.0;
  options.step.project_bounds = true;
  options.step.enable_constraint_autodiff = true;

  pluss::subsp::math::optim::SeptimaConstraints constraints{};
  constraints.equalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].sum() + p[1] - 2.0};
  };
  constraints.inequalities = [](const std::vector<Series>& p) {
    return std::vector<Series>{p[0].square().sum() - 5.0};
  };
  constraints.lbs = {
      pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg),
      pluss::series::undefined(),
  };

  options.progress_hook = [&](const pluss::subsp::math::optim::SeptimaProgressSnapshot& s) {
    if (s.iteration % 50 == 0 || s.iteration == 1) {
      std::cout << "iter=" << s.iteration
                << " obj=" << s.objective
                << " viol=" << s.constraint_violation
                << " grad_inf=" << s.gradient_inf_norm
                << " step_inf=" << s.step_inf_norm << '\n';
    }
    return true;
  };

  const auto fit = pluss::subsp::math::optim::septima(
      [&](const std::vector<Series>& p) -> Series {
        return (((p[0] - w_target).square()).sum() +
                ((p[1] - b_target).square()).sum()) *
               0.5;
      },
      std::vector<Series>{w, b},
      options,
      constraints);

  std::cout << "final w=[" << fit.params[0].select(0, 0).item<double>() << ", "
            << fit.params[0].select(0, 1).item<double>() << "] b=" << as_f64(fit.params[1]) << '\n';

  return 0;
}
