#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

using pluss::ScalarType;
using pluss::Series;

int main() {
  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series target = pluss::make_series({1.0, 2.0}, cfg);
  const Series initial = pluss::make_series({1.0, 1.0}, cfg);

  const auto constraints = pluss::subsp::math::optim::constraints()
                               .lbs(pluss::make_series({0.0, 0.0}, cfg))
                               .ln_ineq(
                                   pluss::make_series({1.0, 1.0}, cfg).reshape({1, 2}),
                                   pluss::make_series({1.5}, cfg))
                               .nl_ineq([](const pluss::subsp::math::optim::ParameterPack<pluss::Series>& p)
                                            -> pluss::Series {
                                 return p.at("x").square().sum() - 3.0;
                               })
                               .build();

  pluss::subsp::math::optim::OptimaOptions options = pluss::subsp::math::optim::presets::sequential_qp();
  options.algorithm = pluss::subsp::math::optim::OptimaAlgorithm::WorkingSet;
  options.working_set.max_iterations = 60;
  options.working_set.constraint_tolerance = 1e-8;
  options.working_set.optimality_tolerance = 1e-6;
  options.progress_hook = pluss::subsp::math::optim::reports::iter(std::cout);

  const auto result = pluss::subsp::math::optim::optima(
      [target](const pluss::Series& x) -> pluss::Series {
        return (x - target).square().sum();
      },
      initial,
      options,
      constraints);

  std::cout << std::fixed << std::setprecision(6);
  pluss::subsp::math::optim::reports::compact_summary(std::cout, result) << '\n';

  const Series x_hat = result.params.at("x").reshape({2});
  std::cout << "x0=" << x_hat.select(0, 0) << '\n';
  std::cout << "x1=" << x_hat.select(0, 1) << '\n';
  return 0;
}
