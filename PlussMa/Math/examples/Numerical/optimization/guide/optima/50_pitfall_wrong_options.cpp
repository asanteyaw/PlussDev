// The most common mistake with `OptimaOptions`: setting an option on the struct of an algorithm you are not
// running. Every algorithm reads only its own struct, so the setting is silently ignored. The default
// algorithm is sequential QP, which makes the mistake easy to make in the other direction too. Both runs
// below ask for "at most 2 iterations"; only one of them does. The second half shows the setting done right:
// sequential QP reduces its quadratic subproblem to a symmetric positive-definite system, so the
// linear-solver and Krylov options on its own struct select how that system is solved and are counted in
// the output.
#include <pluss/ma.h>

#include <iostream>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto start = optim::pack_scalars({{"a", pluss::make_series(0.0).to(f64)}, {"b", pluss::make_series(0.0).to(f64)}});
  auto objective = [](const Pack& p) {
    return optim::square(p.at("a") - 3.0) + optim::square(p.at("b") - 3.0);
  };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::square(p.at("a")) + optim::square(p.at("b")) - 2.0; });

  optim::OptimaOptions wrong;
  wrong.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  wrong.sequential_qp.max_iterations = 2;  // not the struct of the algorithm that runs
  const auto ignored = optim::optima(objective, start, wrong, disc);

  optim::OptimaOptions right;
  right.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  right.internal_protocol.max_iterations = 2;
  const auto honored = optim::optima(objective, start, right, disc);

  std::cout << "set on sequential_qp, running InternalProtocol: " << ignored.iteration << " iterations, flag "
            << static_cast<int>(ignored.exit_flag) << "\n";
  std::cout << "set on internal_protocol, running InternalProtocol: " << honored.iteration << " iterations, flag "
            << static_cast<int>(honored.exit_flag) << "\n";

  // The same problem with the subproblem solved by conjugate gradients.
  optim::OptimaOptions plain;
  plain.algorithm = optim::OptimaAlgorithm::SequentialQP;
  optim::OptimaOptions krylov = plain;
  krylov.sequential_qp.linear_system_solver = optim::OptimaLinearSystemSolver::ConjugateGradient;
  krylov.sequential_qp.max_krylov_iterations = 50;
  krylov.sequential_qp.system_structure = optim::OptimaLinearSystemStructure::Sparse;
  const auto a = optim::optima(objective, start, plain, disc);
  const auto b = optim::optima(objective, start, krylov, disc);
  std::cout << "sequential QP default:       " << a.iteration << " iterations, krylov_iterations "
            << a.output.krylov_iterations << "\n";
  std::cout << "sequential QP with Krylov:   " << b.iteration << " iterations, krylov_iterations "
            << b.output.krylov_iterations << "\n";

  // The default algorithm is not the interior method.
  const optim::OptimaOptions defaults;
  std::cout << "default algorithm is SequentialQP: " << std::boolalpha
            << (defaults.algorithm == optim::OptimaAlgorithm::SequentialQP) << "\n";
  return 0;
}
