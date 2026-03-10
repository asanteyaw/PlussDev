#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

using pluss::ScalarType;
using pluss::Series;

int main() {
  const pluss::Config cfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const Series target = pluss::make_series({1.0, 2.0}, cfg);
  const Series initial = pluss::make_series({1.0, 1.0}, cfg);

  const auto constraints = pluss::optim::constraints()
                               .lbs(pluss::make_series({0.0, 0.0}, cfg))
                               .ln_ineq(
                                   pluss::make_series({1.0, 1.0}, cfg).reshape({1, 2}),
                                   pluss::make_series({1.5}, cfg))
                               .nl_ineq([](const pluss::optim::ParameterPack<pluss::optim::Trace>& p)
                                            -> pluss::optim::Trace {
                                 return pluss::optim::sum(pluss::optim::square(p.at("x"))) - 3.0;
                               })
                               .build();

  pluss::optim::OptimaOptions options = pluss::optim::presets::sequential_qp();
  options.sequential_qp.max_iterations = 60;
  options.sequential_qp.constraint_tolerance = 1e-8;
  options.sequential_qp.optimality_tolerance = 1e-6;
  options.progress_hook = pluss::optim::reports::iter(std::cout);

  const auto result = pluss::optim::optima(
      [target](const Series& x) -> Series {
        return ((x - target) * (x - target)).sum();
      },
      initial,
      options,
      constraints);

  std::cout << std::fixed << std::setprecision(6);
  pluss::optim::reports::compact_summary(std::cout, result) << '\n';

  const Series x_hat = result.params.at("x").reshape({2});
  std::cout << "x0=" << x_hat.select(0, 0) << '\n';
  std::cout << "x1=" << x_hat.select(0, 1) << '\n';
  return 0;
}
