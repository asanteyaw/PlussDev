// Scaling, measured. The model has an objective and constraints written in units that differ by a factor K from
// the natural ones (K = 1e3, 1e6, 1e9): think of money in cents against money in billions. `problem_scaling`
// decides what the solver does about it. `None` and `InitialPoint` solve the problem as written (the second also
// rescales the variables); `ObjectiveAndConstraints`, the default, solves a rescaled copy in which the objective and
// every constraint row have a gradient of reasonable size at the start, and reports the answer in the original
// units. Each cell is the exit flag and the iteration count; the last column is the only one that converges
// everywhere, at the price of more iterations where the units were already fine.
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
          return K * (p.at("a") - 1.0).square() + (1.0 / K) * (p.at("b") - 1500.0).square() + K * 0.01 * p.at("a");
        };
        const auto cons =
            optim::constraints()
                .nl_eq([K](const Pack& p) { return K * (p.at("a").square() + (p.at("b") * 0.001).square() - 3.0); })
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
