#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

using pluss::ScalarType;
using pluss::Series;
using pluss::optim::OptimaOptions;

const pluss::Config kDemoCfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

pluss::optim::Trace trace_quadratic_objective(const pluss::optim::Trace& x) {
  const Series target = pluss::make_series({0.30, -0.40}, kDemoCfg).to(x.value().platform());
  const auto diff = x - pluss::optim::trace(target, false);
  return pluss::optim::sum(pluss::optim::square(diff));
}

int main() {
  const pluss::Config cfg = kDemoCfg;

  const Series target = pluss::make_series({0.30, -0.40}, cfg);
  const Series initial = pluss::make_series({0.80, -0.80}, cfg);
  const auto trace_quadratic_objective_lambda = [target](const pluss::optim::Trace& x) -> pluss::optim::Trace {
    const auto diff = x - pluss::optim::trace(target, false);
    return pluss::optim::sum(pluss::optim::square(diff));
  };

  OptimaOptions options = pluss::optim::presets::internal_protocol();
  options.internal_protocol.max_iterations = 80;
  options.internal_protocol.constraint_tolerance = 1e-8;
  options.internal_protocol.optimality_tolerance = 1e-8;

  const auto constraints = pluss::optim::constraints()
                               .lbs(pluss::make_series({-1.0, -1.0}, cfg))
                               .ubs(pluss::make_series({1.0, 1.0}, cfg))
                               .ln_ineq(
                                   pluss::make_series({1.0, 1.0}, cfg).reshape({1, 2}),
                                   pluss::make_series({0.5}, cfg))
                               .nl_ineq([](const pluss::optim::ParameterPack<pluss::optim::Trace>& p)
                                            -> pluss::optim::Trace {
                                 return pluss::optim::sum(pluss::optim::square(p.at("x"))) - 1.0;
                               })
                               .build();

  const auto result_fn = pluss::optim::optima(trace_quadratic_objective, initial, options, constraints);
  const auto result_lambda = pluss::optim::optima(trace_quadratic_objective_lambda, initial, options, constraints);
  const Series x_fn = result_fn.params.at("x").reshape({2});
  const Series x_lambda = result_lambda.params.at("x").reshape({2});

  std::cout << std::fixed << std::setprecision(6);
  std::cout << "[function objective]\n";
  pluss::optim::reports::compact_summary(std::cout, result_fn) << '\n';
  std::cout << "estimated x0=" << x_fn.select(0, 0) << '\n';
  std::cout << "estimated x1=" << x_fn.select(0, 1) << '\n';
  std::cout << "[lambda objective]\n";
  pluss::optim::reports::compact_summary(std::cout, result_lambda) << '\n';
  std::cout << "estimated x0=" << x_lambda.select(0, 0) << '\n';
  std::cout << "estimated x1=" << x_lambda.select(0, 1) << '\n';
  std::cout << "target x0=" << target.select(0, 0) << '\n';
  std::cout << "target x1=" << target.select(0, 1) << '\n';
  return 0;
}
