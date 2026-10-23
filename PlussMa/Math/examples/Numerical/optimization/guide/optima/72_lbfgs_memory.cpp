// The limited-memory model rebuilds its curvature from the last `lbfgs_memory` step pairs. A longer memory is a
// better model and a costlier one. The table solves the 12-variable chain with the interior method for memories
// from 1 to 30. The objective at the answer shows the accuracy reached, which also improves with memory.
#include <pluss/ma.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;
using Block = optim::ParameterPack<pluss::Series>::Block;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

// The Rosenbrock chain: n variables, strongly coupled curvature, boxed to [-3, 3], started at (-1.2, 1, -1.2, ...).
struct Chain {
  int n;
  std::string name(int i) const { return "x" + std::to_string(i); }
  optim::ParameterPack<pluss::Series> filled(double lo_or_hi) const {
    std::vector<Block> b;
    for (int i = 0; i < n; ++i) b.push_back({name(i), scalar(lo_or_hi).reshape({1})});
    return optim::ParameterPack<pluss::Series>(b);
  }
  optim::ParameterPack<pluss::Series> start() const {
    std::vector<Block> b;
    for (int i = 0; i < n; ++i) b.push_back({name(i), scalar(i % 2 == 0 ? -1.2 : 1.0).reshape({1})});
    return optim::ParameterPack<pluss::Series>(b);
  }
  optim::Trace objective(const Pack& p) const {
    optim::Trace total = p.at(name(0)) * 0.0;
    for (int i = 0; i + 1 < n; ++i)
      total = total + 100.0 * optim::square(p.at(name(i + 1)) - optim::square(p.at(name(i)))) + optim::square(1.0 - p.at(name(i)));
    return total;
  }
  auto box() const { return optim::constraints().lbs(filled(-3.0)).ubs(filled(3.0)); }
  optim::OptimaState solve(const optim::OptimaOptions& o) const {
    return optim::optima([this](const Pack& p) { return objective(p); }, start(), o, box());
  }
};

int main() {
  std::cout << std::left << std::setw(10) << "memory" << std::setw(6) << "flag" << std::setw(7) << "iters" << std::setw(7)
            << "evals" << "objective\n";
  for (std::int64_t memory : {1, 3, 5, 10, 30}) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    o.internal_protocol.hessian_approximation = optim::OptimaHessianApproximation::Lbfgs;
    o.internal_protocol.lbfgs_memory = memory;
    o.internal_protocol.max_iterations = 300;
    const auto r = Chain{12}.solve(o);
    std::cout << std::left << std::setw(10) << memory << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7)
              << r.iteration << std::setw(7) << r.function_evaluations << std::scientific << std::setprecision(2)
              << r.evaluation.value.template item<double>() << "\n";
  }
  return 0;
}
