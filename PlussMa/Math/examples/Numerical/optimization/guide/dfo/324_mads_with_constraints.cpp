// Mesh adaptive search with constraints. The same disc problem as Program 1, run with pattern search and with
// OrthoMADS. Both return a feasible point. When constraints are present the MADS solvers use a progressive barrier,
// which treats a point that violates a constraint as acceptable only if the violation is shrinking; the answer
// approaches the boundary optimum from inside rather than landing on it exactly.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::cout << std::left << std::setw(16) << "solver" << std::setw(26) << "status" << std::setw(12) << "objective"
            << std::setw(12) << "violation" << "evals\n";
  for (auto k : {dfo::DfoSolverKind::PatternSearch, dfo::DfoSolverKind::OrthoMads, dfo::DfoSolverKind::LtMads}) {
    dfo::DfoProblem p;
    p.initial = vec({0.0, 0.0});
    p.objective = [](const pluss::Series& x) { return (x - vec({2.0, 2.0})).square().sum(); };
    p.constraints = [](const pluss::Series& x) { return (x.square().sum() - 1.0).reshape({1}); };
    dfo::DfoOptions o;
    o.solver = k;
    const auto r = dfo::DFO::solve(p, o);
    std::cout << std::left << std::setw(16) << dfo::to_string(k) << std::setw(26) << dfo::to_string(r.status) << std::fixed
              << std::setprecision(4) << std::setw(12) << r.minimum << std::scientific << std::setprecision(1) << std::setw(12)
              << r.diagnostics.max_constraint_violation << std::defaultfloat << r.diagnostics.evaluations << "\n";
  }
  std::cout << "exact objective 3.3431\n";
}
