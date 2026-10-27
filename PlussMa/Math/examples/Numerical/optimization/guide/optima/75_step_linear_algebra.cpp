// Each step ends in a linear system, and the solver for it is a choice. The problem is a smoothing fit with 100
// variables, 198 linear inequalities, one equality and bounds. Four solvers are tried in both quadratic-model
// algorithms: Cholesky and pivoted LU factorize the system; conjugate gradients never form it and count their
// iterations. All reach the same optimum. Conjugate gradients exist for the
// sizes where a dense factorization no longer fits (see `iterative_dimension_threshold`).
#include <pluss/ma.h>

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

int main() {
  const int n = 100;
  std::vector<double> target(n), start(n);
  double total = 0.0;
  for (int i = 0; i < n; ++i) {
    target[i] = 0.2 + 0.6 * i / (n - 1) + 0.15 * std::sin(1.7 * i);
    start[i] = (i % 2 == 0) ? 0.9 : 0.1;
    total += target[i];
  }
  std::vector<double> diff(static_cast<std::size_t>(2 * (n - 1) * n), 0.0), cap(static_cast<std::size_t>(2 * (n - 1)), 0.12);
  for (int i = 0; i + 1 < n; ++i) {
    diff[static_cast<std::size_t>((2 * i) * n + i)] = 1.0;
    diff[static_cast<std::size_t>((2 * i) * n + i + 1)] = -1.0;
    diff[static_cast<std::size_t>((2 * i + 1) * n + i)] = -1.0;
    diff[static_cast<std::size_t>((2 * i + 1) * n + i + 1)] = 1.0;
  }
  const auto goal = vec(target);
  auto objective = [&](const pluss::Series& x) { return (x - goal).square().sum(); };
  const auto constraints =
      optim::constraints()
          .ln_ineq(pluss::make_series(diff, {2 * (n - 1), n}).to(f64), vec(cap))
          .ln_eq(pluss::make_series(std::vector<double>(n, 1.0), {1, n}).to(f64), vec({total}))
          .lbs(vec(std::vector<double>(n, 0.0)))
          .ubs(vec(std::vector<double>(n, 1.0)));

  struct Row { const char* label; optim::OptimaLinearSystemSolver solver; optim::OptimaLinearSystemStructure structure; };
  const Row rows[] = {
      {"Cholesky", optim::OptimaLinearSystemSolver::Cholesky, optim::OptimaLinearSystemStructure::Dense},
      {"PivotedDirect", optim::OptimaLinearSystemSolver::PivotedDirect, optim::OptimaLinearSystemStructure::Dense},
      {"ConjugateGradient", optim::OptimaLinearSystemSolver::ConjugateGradient, optim::OptimaLinearSystemStructure::Sparse}};
  std::cout << std::left << std::setw(18) << "solver" << std::setw(18) << "algorithm" << std::setw(6) << "flag" << std::setw(7)
            << "iters" << std::setw(11) << "objective" << "krylov\n";
  for (const auto& row : rows) {
    for (const auto alg : {optim::OptimaAlgorithm::SequentialQP, optim::OptimaAlgorithm::InternalProtocol}) {
      optim::OptimaOptions o;
      o.algorithm = alg;
      o.sequential_qp.linear_system_solver = row.solver;
      o.sequential_qp.system_structure = row.structure;
      o.internal_protocol.linear_system_solver = row.solver;
      o.internal_protocol.system_structure = row.structure;
      const auto r = optim::optima(objective, vec(start), o, constraints);
      std::cout << std::left << std::setw(18) << row.label << std::setw(18)
                << (alg == optim::OptimaAlgorithm::SequentialQP ? "SequentialQP" : "InternalProtocol") << std::setw(6)
                << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration << std::fixed << std::setprecision(5)
                << std::setw(11) << r.evaluation.value.template item<double>() << std::setw(9) << r.output.krylov_iterations
                << "\n";
    }
  }
  return 0;
}
