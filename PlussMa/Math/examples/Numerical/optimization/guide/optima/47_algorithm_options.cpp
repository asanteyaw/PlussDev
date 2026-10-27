// Each algorithm has its own options struct inside `OptimaOptions`. Most have little effect on a well-behaved
// problem; a few decide the outcome. This program sweeps the options that moved the result on one standard
// problem and prints the work and the answer for each setting. The objective value tells you whether a
// setting also changed which local minimum was found: 17.0140 is the best known, 27.1464 is another point
// that satisfies the first-order conditions.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto four = [&](double a, double b, double c, double d) {
    return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
  };
  auto objective = [](const Pack& p) {
    return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
  };
  const auto constraints =
      optim::constraints()
          .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
          .nl_eq([](const Pack& p) {
            return p.at("a").square() + p.at("b").square() + p.at("c").square() +
                   p.at("d").square() - 40.0;
          })
          .lbs(four(1, 1, 1, 1))
          .ubs(four(5, 5, 5, 5));

  std::cout << std::left << std::setw(52) << "setting" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(7) << "evals" << "objective\n";
  auto fmt = [](double v) {
    std::ostringstream out;
    out << v;
    return out.str();
  };
  auto run = [&](const std::string& label, const optim::OptimaOptions& options) {
    const auto r = optim::optima(objective, four(1, 5, 5, 1), options, constraints);
    std::cout << std::left << std::setw(52) << label << std::setw(6) << static_cast<int>(r.exit_flag)
              << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations << std::fixed
              << std::setprecision(4) << r.evaluation.value.item<double>() << std::defaultfloat << "\n";
  };

  for (double mu : {1e-4, 1e-2, 1.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    o.internal_protocol.initial_barrier_parameter = mu;
    run("InternalProtocol initial_barrier_parameter " + fmt(mu), o);
  }
  for (double f : {0.9, 0.99}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    o.internal_protocol.fraction_to_boundary = f;
    run("InternalProtocol fraction_to_boundary " + fmt(f), o);
  }
  for (bool on : {true, false}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    o.internal_protocol.independent_nonlinear_slacks = on;
    run(std::string("InternalProtocol independent_nonlinear_slacks ") + (on ? "true" : "false"), o);
  }
  for (double tol : {1e-3, 1e-6, 1e-10}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.optimality_tolerance = tol;
    run("SequentialQP optimality_tolerance " + fmt(tol), o);
  }
  for (double penalty : {1.0, 10.0, 100.0}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::MeritMethod;
    o.penalty.penalty = penalty;
    run("MeritMethod penalty " + fmt(penalty), o);
  }
  return 0;
}
