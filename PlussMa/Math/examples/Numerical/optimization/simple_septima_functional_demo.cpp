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

  const Series p = pluss::make_series(std::vector<double>{0.5, -0.5}, {2}, cfg);
  const Series target = pluss::make_series(std::vector<double>{2.0, 1.0}, {2}, cfg);

  pluss::subsp::math::optim::SeptimaSolveOptions opts{};
  opts.max_iterations = 120;
  opts.gradient_tolerance = 1e-8;
  opts.constraint_tolerance = 1e-8;
  opts.step.adam.learning_rate = 0.04;
  opts.step.dual_learning_rate = 0.08;
  opts.step.equality_penalty = 6.0;
  opts.step.inequality_penalty = 6.0;

  pluss::subsp::math::optim::SeptimaConstraints c{};
  c.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].sum() - 3.0}; };
  c.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].square().sum() - 10.0}; };
  c.lbs = {pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg)};

  const auto fit = pluss::subsp::math::optim::septima(
      [&](const Series& v) -> Series {
        return ((v - target).square()).sum() *
               0.5;
      },
      p,
      opts,
      c);

  std::cout << "fit=[" << fit.params[0].select(0, 0).item<double>() << ", "
            << fit.params[0].select(0, 1).item<double>() << "]\n";
  return 0;
}
