<!-- generated from units_measured.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Units, measured

[Back to Curvature, Linear Algebra and Scaling](../optima_06_curvature_linear_algebra_scaling.md)


The model has an objective and constraints written in units a factor `K` away from the natural ones. The table is exit flag and iteration count for every algorithm under every `problem_scaling` mode.

```cpp
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

int main() {
  struct Alg { const char* label; optim::OptimaAlgorithm alg; };
  const Alg algs[] = {{"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                      {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};
  struct Mode { const char* label; optim::OptimaProblemScaling mode; };
  const Mode modes[] = {{"None", optim::OptimaProblemScaling::None},
                        {"InitialPoint", optim::OptimaProblemScaling::InitialPoint},
                        {"ObjectiveAndConstraints", optim::OptimaProblemScaling::ObjectiveAndConstraints}};
  std::cout << "cell = exit flag / iterations   (1 = converged)\n";
  std::cout << std::left << std::setw(8) << "K" << std::setw(18) << "algorithm";
  for (const auto& m : modes) std::cout << std::setw(26) << m.label;
  std::cout << "\n";
  for (double K : {1e3, 1e6, 1e9}) {
    for (const auto& a : algs) {
      std::ostringstream k;
      k << K;
      std::cout << std::left << std::setw(8) << k.str() << std::setw(18) << a.label;
      for (const auto& m : modes) {
        auto f = [K](const Pack& p) {
          return K * optim::square(p.at("a") - 1.0) + (1.0 / K) * optim::square(p.at("b") - 1500.0) + K * 0.01 * p.at("a");
        };
        const auto cons =
            optim::constraints()
                .nl_eq([K](const Pack& p) { return K * (optim::square(p.at("a")) + optim::square(p.at("b") * 0.001) - 3.0); })
                .nl_ineq([K](const Pack& p) { return (p.at("b") - 1400.0) / K; });
        optim::OptimaOptions o;
        o.algorithm = a.alg;
        o.sequential_qp.problem_scaling = m.mode;
        o.working_set.problem_scaling = m.mode;
        o.internal_protocol.problem_scaling = m.mode;
        o.sequential_qp.max_iterations = 300;
        o.working_set.max_iterations = 300;
        o.internal_protocol.max_iterations = 300;
        const auto r = optim::optima(f, optim::pack_scalars({{"a", scalar(0.3)}, {"b", scalar(900.0)}}), o, cons);
        std::cout << std::setw(26) << (std::to_string(static_cast<int>(r.exit_flag)) + " / " + std::to_string(r.iteration));
      }
      std::cout << "\n";
    }
  }
  return 0;
}
```

Output

```text
cell = exit flag / iterations   (1 = converged)
K       algorithm         None                      InitialPoint              ObjectiveAndConstraints   
1000    SequentialQP      1 / 10                    1 / 10                    1 / 16                    
1000    WorkingSet        1 / 43                    1 / 10                    1 / 23                    
1000    InternalProtocol  1 / 17                    1 / 17                    1 / 29                    
1e+06   SequentialQP      1 / 9                     1 / 9                     1 / 17                    
1e+06   WorkingSet        1 / 58                    1 / 60                    1 / 24                    
1e+06   InternalProtocol  -6 / 15                   -6 / 15                   1 / 49                    
1e+09   SequentialQP      -6 / 33                   -6 / 33                   1 / 19                    
1e+09   WorkingSet        1 / 11                    -6 / 11                   1 / 35                    
1e+09   InternalProtocol  -6 / 20                   -6 / 20                   1 / 48                    
```
With `None`, the problem as written, sequential QP converges at `K = 1000` and `1e6` but fails at `1e9` (flag `-6`), and the interior method fails at both `1e6` and `1e9`; only the working-set method survives all three, in 43, 58 and 11 iterations. `InitialPoint` changes nothing for sequential QP and the interior method and makes the working-set method fail at `1e9`. `ObjectiveAndConstraints`, the default, is the only column in which every cell converges: sequential QP in 16, 17 and 19 iterations, the working-set method in 23, 24 and 35, and the interior method in 29, 49 and 48. The price is visible where the units were already fine: at `K = 1000` sequential QP takes 16 iterations instead of 10. The automatic rescaling is no substitute for a model whose numbers mean something, but it removes the failure that extreme units used to cause.
