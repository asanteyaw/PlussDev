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

  const std::vector<Series> p{
      pluss::make_series(std::vector<double>{0.25, 0.25}, {2}, cfg),
      pluss::make_series(std::vector<double>{-1.0}, {1}, cfg),
  };
  const Series wt = pluss::make_series(std::vector<double>{2.0, 1.0}, {2}, cfg);
  const Series bt = pluss::make_series(std::vector<double>{0.5}, {1}, cfg);

  pluss::optim::SeptimaSolveOptions opts{};
  opts.max_iterations = 200;
  opts.gradient_tolerance = 1e-8;
  opts.constraint_tolerance = 1e-8;
  opts.step.adam.learning_rate = 0.03;
  opts.step.adam.weight_decay = 1e-3;
  opts.step.dual_learning_rate = 0.08;
  opts.step.equality_penalty = 8.0;
  opts.step.inequality_penalty = 8.0;

  pluss::optim::SeptimaConstraints c{};
  c.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].sum() + x[1] - 2.0}; };
  c.equalities_trace = [](const std::vector<Trace>& x) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(x[0]) + x[1] - 2.0};
  };
  c.inequalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].square().sum() - 5.0}; };
  c.inequalities_trace = [](const std::vector<Trace>& x) {
    return std::vector<Trace>{pluss::subsp::math::diff::sum(pluss::subsp::math::diff::square(x[0])) - 5.0};
  };
  c.lbs = {pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg), Series::undefined()};

  const auto fit = pluss::optim::septima(
      [&](const std::vector<Trace>& x) -> Trace {
        return (pluss::subsp::math::diff::sum(
                    pluss::subsp::math::diff::square(x[0] - pluss::subsp::math::diff::fixed(wt))) +
                pluss::subsp::math::diff::sum(
                    pluss::subsp::math::diff::square(x[1] - pluss::subsp::math::diff::fixed(bt)))) *
               0.5;
      },
      p,
      opts,
      c);

  std::cout << "fit w=[" << fit.params[0].select(0, 0).item<double>() << ", "
            << fit.params[0].select(0, 1).item<double>()
            << "] b=" << fit.params[1].item<double>() << '\n';
  return 0;
}
