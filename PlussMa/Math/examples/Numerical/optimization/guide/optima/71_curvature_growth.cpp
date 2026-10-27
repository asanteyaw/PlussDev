// How the curvature model scales with the number of variables. The Rosenbrock chain is solved for 6, 12 and 24
// variables with the damped dense BFGS update, the limited-memory update (interior method only) and the identity,
// which uses no curvature at all. Each cell is iterations / function evaluations, and an asterisk marks a run that
// did not converge within 300 iterations. The real models grow gently with n; the identity does not finish at
// any size.
#include <pluss/ma.h>

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
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
  pluss::Series objective(const Pack& p) const {
    pluss::Series total = p.at(name(0)) * 0.0;
    for (int i = 0; i + 1 < n; ++i)
      total = total + 100.0 * (p.at(name(i + 1)) - p.at(name(i)).square()).square() + (1.0 - p.at(name(i))).square();
    return total;
  }
  auto box() const { return optim::constraints().lbs(filled(-3.0)).ubs(filled(3.0)); }
  optim::OptimaState solve(const optim::OptimaOptions& o) const {
    return optim::optima([this](const Pack& p) { return objective(p); }, start(), o, box());
  }
};

int main() {
  struct Col { const char* label; optim::OptimaAlgorithm alg; optim::OptimaHessianApproximation model; };
  const Col cols[] = {
      {"SequentialQP Bfgs", optim::OptimaAlgorithm::SequentialQP, optim::OptimaHessianApproximation::Bfgs},
      {"Interior Bfgs", optim::OptimaAlgorithm::InternalProtocol, optim::OptimaHessianApproximation::Bfgs},
      {"Interior Lbfgs", optim::OptimaAlgorithm::InternalProtocol, optim::OptimaHessianApproximation::Lbfgs},
      {"Interior Identity", optim::OptimaAlgorithm::InternalProtocol, optim::OptimaHessianApproximation::Identity}};
  std::cout << std::left << std::setw(8) << "n";
  for (const auto& c : cols) std::cout << std::setw(22) << c.label;
  std::cout << "\n";
  for (int n : {6, 12, 24}) {
    std::cout << std::left << std::setw(8) << n;
    for (const auto& c : cols) {
      optim::OptimaOptions o;
      o.algorithm = c.alg;
      o.sequential_qp.hessian_approximation = c.model;
      o.internal_protocol.hessian_approximation = c.model;
      o.sequential_qp.max_iterations = 300;
      o.internal_protocol.max_iterations = 300;
      const auto r = Chain{n}.solve(o);
      std::cout << std::setw(22)
                << (std::to_string(r.iteration) + " / " + std::to_string(r.function_evaluations) +
                    (r.exit_flag == optim::OptimaExitFlag::Converged ? "" : " *"));
    }
    std::cout << "\n";
  }
  std::cout << "* did not converge\n";
  return 0;
}
