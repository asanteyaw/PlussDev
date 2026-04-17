#include <pluss/pluss.h>
#include <pluss/ma.h>
#include <pluss/subsp/math/diff.h>

#include <iostream>
#include <vector>

int main() {
  using pluss::Config;
  using pluss::ScalarType;
  using pluss::Series;
  using pluss::subsp::math::diff::Trace;

  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series p = pluss::make_series(std::vector<double>{0.5, -0.5}, {2}, cfg);
  const Series target = pluss::make_series(std::vector<double>{2.0, 1.0}, {2}, cfg);

  pluss::optim::MinimaxSolveOptions opts{};
  opts.max_iterations = 120;
  opts.gradient_tolerance = 1e-8;
  opts.constraint_tolerance = 1e-8;
  opts.step.adam.learning_rate = 0.04;
  opts.step.dual_learning_rate = 0.08;
  opts.step.equality_penalty = 6.0;
  opts.step.inequality_penalty = 6.0;

  pluss::optim::MinimaxConstraints c{};
  c.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].sum() - 3.0}; };
  c.equalities_trace = [](const std::vector<Trace>& x) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(x[0]) - 3.0};
  };
  c.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].square().sum() - 10.0}; };
  c.inequalities_trace = [](const std::vector<Trace>& x) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(pluss::subsp::math::diff::square(x[0])) - 10.0};
  };
  c.lbs = {pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg)};

  const auto fit = pluss::optim::minimax(
      [&](const Trace& v) -> Trace {
        return pluss::subsp::math::diff::sum(
                   pluss::subsp::math::diff::square(v - pluss::subsp::math::diff::fixed(target))) *
               0.5;
      },
      p,
      opts,
      c);

  std::cout << "fit=[" << fit.params[0].select(0, 0).item<double>() << ", "
            << fit.params[0].select(0, 1).item<double>() << "]\n";
  return 0;
}
