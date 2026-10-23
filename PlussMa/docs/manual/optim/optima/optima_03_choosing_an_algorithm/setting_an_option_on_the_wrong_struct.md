<!-- generated from setting_an_option_on_the_wrong_struct.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Setting an option on the wrong struct

[Back to Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)


Each algorithm reads only its own struct, so a setting made on another algorithm's struct is silently ignored. The default algorithm is sequential QP, which makes the mistake easy in both directions. The program then shows the setting done right: sequential QP reduces its quadratic subproblem to a symmetric positive-definite system, and the linear-solver and Krylov options on its own struct choose how that system is solved (Cholesky by default, conjugate gradients on request or for large problems).

```cpp
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
```

Output

```text
set on sequential_qp, running InternalProtocol: 6 iterations, flag 1
set on internal_protocol, running InternalProtocol: 2 iterations, flag 0
sequential QP default:       5 iterations, krylov_iterations 0
sequential QP with Krylov:   5 iterations, krylov_iterations 88
default algorithm is SequentialQP: true
```
The first run asked for at most 2 iterations on the wrong struct and ran 6 to convergence; the second asked on the right struct and stopped at 2 with flag `0`. Asking for conjugate gradients on the sequential QP struct changed how the subproblems were solved but not the path: both runs take 5 iterations, and `krylov_iterations` is `0` for the default and `88` when conjugate gradients were requested. When a setting seems to have no effect, check that it sits on the struct of the algorithm that is running.
