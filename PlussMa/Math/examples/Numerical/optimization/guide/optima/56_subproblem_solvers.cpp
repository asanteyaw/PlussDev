// Each iteration solves a quadratic subproblem, reduced to a symmetric positive-definite system in the
// variables. Which solver handles that system is a choice: a Cholesky factorization (the default for small
// problems), a pivoted direct solve, an explicit inverse or pseudo-inverse, or conjugate gradients, which never
// forms the matrix and is meant for problems with many variables. All of them reach the same answer here; the
// last column counts the conjugate-gradient iterations, which is nonzero only when that solver ran.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }
optim::ParameterPack<pluss::Series> four(double a, double b, double c, double d) {
  return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
}
auto objective = [](const Pack& p) { return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c"); };
auto constraints() {
  return optim::constraints()
      .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
      .nl_eq([](const Pack& p) {
        return optim::square(p.at("a")) + optim::square(p.at("b")) + optim::square(p.at("c")) + optim::square(p.at("d")) - 40.0;
      })
      .lbs(four(1, 1, 1, 1))
      .ubs(four(5, 5, 5, 5));
}

int main() {
  struct Row { const char* name; optim::OptimaLinearSystemSolver solver; optim::OptimaLinearSystemStructure structure; };
  const Row rows[] = {
      {"Auto", optim::OptimaLinearSystemSolver::Auto, optim::OptimaLinearSystemStructure::Auto},
      {"Cholesky", optim::OptimaLinearSystemSolver::Cholesky, optim::OptimaLinearSystemStructure::Dense},
      {"PivotedDirect", optim::OptimaLinearSystemSolver::PivotedDirect, optim::OptimaLinearSystemStructure::Dense},
      {"Inverse", optim::OptimaLinearSystemSolver::Inverse, optim::OptimaLinearSystemStructure::Dense},
      {"PseudoInverse", optim::OptimaLinearSystemSolver::PseudoInverse, optim::OptimaLinearSystemStructure::Dense},
      {"ConjugateGradient", optim::OptimaLinearSystemSolver::ConjugateGradient, optim::OptimaLinearSystemStructure::Sparse},
  };
  std::cout << std::left << std::setw(20) << "solver" << std::setw(7) << "iters" << std::setw(14) << "objective"
            << "krylov iterations\n";
  for (const auto& row : rows) {
    optim::OptimaOptions o;
    o.algorithm = optim::OptimaAlgorithm::SequentialQP;
    o.sequential_qp.linear_system_solver = row.solver;
    o.sequential_qp.system_structure = row.structure;
    const auto r = optim::optima(objective, four(1, 5, 5, 1), o, constraints());
    std::cout << std::left << std::setw(20) << row.name << std::setw(7) << r.iteration << std::fixed << std::setprecision(6)
              << std::setw(14) << r.evaluation.value.item<double>() << r.output.krylov_iterations << "\n";
  }
  return 0;
}
