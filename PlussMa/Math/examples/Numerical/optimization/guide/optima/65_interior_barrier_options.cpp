// The barrier schedule: where the barrier parameter starts, how fast it falls, and where it stops. A large
// start keeps the iterates well inside and makes early steps cautious; a small start behaves like a nearly
// unpenalized Newton method and risks being pushed against a wall. `barrier_decay` is the factor applied when a
// barrier problem is solved; `fraction_to_boundary` limits how close to the boundary a single step may land
// (0.99 means at most 99% of the way to the nearest wall).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// The same four-variable problem as in chapter 3: one nonlinear inequality, one nonlinear equality, bounds.
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
auto hs71 = [](const Pack& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); };
auto hs71_constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
      .nl_eq([](const Pack& p) {
        return optim::square(p.at("a")) + optim::square(p.at("b")) + optim::square(p.at("c")) + optim::square(p.at("d")) - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  std::cout << std::left << std::setw(44) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters" << std::setw(7)
            << "evals" << "objective\n";
  auto run = [&](const std::string& label, optim::OptimaOptions o) {
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    const auto r = optim::optima(hs71, four(1, 5, 5, 1), o, hs71_constraints());
    std::cout << std::left << std::setw(44) << label << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7)
              << r.iteration << std::setw(7) << r.function_evaluations << std::fixed << std::setprecision(4)
              << r.evaluation.value.template item<double>() << "\n";
  };
  run("defaults", {});
  for (double mu : {1e-6, 1e-4, 1e-2, 1.0, 10.0}) {
    optim::OptimaOptions o;
    o.internal_protocol.initial_barrier_parameter = mu;
    std::ostringstream l; l << "initial_barrier_parameter " << mu;
    run(l.str(), o);
  }
  for (double d : {0.05, 0.2, 0.5, 0.9}) {
    optim::OptimaOptions o;
    o.internal_protocol.barrier_decay = d;
    std::ostringstream l; l << "barrier_decay " << d;
    run(l.str(), o);
  }
  for (double f : {0.9, 0.99, 0.999}) {
    optim::OptimaOptions o;
    o.internal_protocol.fraction_to_boundary = f;
    std::ostringstream l; l << "fraction_to_boundary " << f;
    run(l.str(), o);
  }
  return 0;
}
