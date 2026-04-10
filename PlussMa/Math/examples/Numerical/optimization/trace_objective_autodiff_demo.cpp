#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

using pluss::ScalarType;
using pluss::Series;
using pluss::optim::OptimaOptions;

const pluss::Config kDemoCfg = pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

pluss::optim::Trace trace_quadratic_objective(
    const pluss::optim::ParameterPack<pluss::optim::Trace>& p) {
  const auto x0 = p.at("x0");
  const auto x1 = p.at("x1");
  const auto t0 = pluss::optim::trace(pluss::make_series(0.30, kDemoCfg), false);
  const auto t1 = pluss::optim::trace(pluss::make_series(-0.40, kDemoCfg), false);
  return pluss::optim::square(x0 - t0) + pluss::optim::square(x1 - t1);
}

int main() {
  const pluss::Config cfg = kDemoCfg;

  const Series target = pluss::make_series({0.30, -0.40}, cfg);
  const auto initial = pluss::optim::pack_scalars({
      {"x0", pluss::make_series(0.80, cfg)},
      {"x1", pluss::make_series(-0.80, cfg)},
  });
  const auto trace_quadratic_objective_lambda =
      [target](const pluss::optim::ParameterPack<pluss::optim::Trace>& p) -> pluss::optim::Trace {
    const auto x0 = p.at("x0");
    const auto x1 = p.at("x1");
    const auto t0 = pluss::optim::trace(target.select(0, 0), false);
    const auto t1 = pluss::optim::trace(target.select(0, 1), false);
    return pluss::optim::square(x0 - t0) + pluss::optim::square(x1 - t1);
  };

  OptimaOptions options = pluss::optim::presets::internal_protocol();
  options.internal_protocol.max_iterations = 80;
  options.internal_protocol.constraint_tolerance = 1e-8;
  options.internal_protocol.optimality_tolerance = 1e-8;

  const auto constraints = pluss::optim::constraints()
                               .lbs(pluss::optim::pack_scalars({
                                   {"x0", pluss::make_series(-1.0, cfg)},
                                   {"x1", pluss::make_series(-1.0, cfg)},
                               }))
                               .ubs(pluss::optim::pack_scalars({
                                   {"x0", pluss::make_series(1.0, cfg)},
                                   {"x1", pluss::make_series(1.0, cfg)},
                               }))
                               .ln_ineq(
                                   pluss::make_series({1.0, 1.0}, cfg).reshape({1, 2}),
                                   pluss::make_series({0.5}, cfg))
                               .nl_ineq([](const pluss::optim::ParameterPack<pluss::optim::Trace>& p)
                                            -> pluss::optim::Trace {
                                 return pluss::optim::square(p.at("x0")) + pluss::optim::square(p.at("x1")) - 1.0;
                               })
                               .build();

  const auto result_fn = pluss::optim::optima(trace_quadratic_objective, initial, options, constraints);
  const auto result_lambda = pluss::optim::optima(trace_quadratic_objective_lambda, initial, options, constraints);
  const Series x_fn0 = result_fn.params.at("x0");
  const Series x_fn1 = result_fn.params.at("x1");
  const Series x_lambda0 = result_lambda.params.at("x0");
  const Series x_lambda1 = result_lambda.params.at("x1");

  std::cout << std::fixed << std::setprecision(6);
  std::cout << "[function objective]\n";
  pluss::optim::reports::compact_summary(std::cout, result_fn) << '\n';
  std::cout << "estimated x0=" << x_fn0 << '\n';
  std::cout << "estimated x1=" << x_fn1 << '\n';
  std::cout << "[lambda objective]\n";
  pluss::optim::reports::compact_summary(std::cout, result_lambda) << '\n';
  std::cout << "estimated x0=" << x_lambda0 << '\n';
  std::cout << "estimated x1=" << x_lambda1 << '\n';
  std::cout << "target x0=" << target.select(0, 0) << '\n';
  std::cout << "target x1=" << target.select(0, 1) << '\n';
  return 0;
}
