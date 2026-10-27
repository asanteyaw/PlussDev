// What "interior" buys. Bounds and linear inequality rows are kept strictly satisfied at every iterate of the
// interior method; sequential QP linearizes them and may step across them on the way. The program records the
// smallest margin to a bound, over all iterates, for a problem whose objective is only defined for x > 0 (a log, with its minimum at x = 0.1), started far away at (9, 9):
// a step that crosses x = 0 would evaluate the log of a non-positive number. The interior method's smallest
// margin is positive. Sequential QP projects its steps onto the bounds, lands on x = 0 to within rounding, and the
// log there is huge, which ruins the line search.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;
const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto f = [](const pluss::Series& x) { return x.sum() - 0.1 * x.log().sum(); };
  const auto cons = optim::constraints().lbs(vec({0.0, 0.0})).ubs(vec({10.0, 10.0}));
  const std::pair<const char*, optim::OptimaAlgorithm> algs[] = {
      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}, {"SequentialQP", optim::OptimaAlgorithm::SequentialQP}};
  std::cout << std::left << std::setw(18) << "algorithm" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(24) << "smallest value of x" << "objective\n";
  for (const auto& [name, alg] : algs) {
    optim::OptimaOptions o;
    o.algorithm = alg;
    double smallest = 1e300;
    o.progress_hook = [&](const optim::OptimaProgressSnapshot& s) {
      for (double v : s.params.at("x").host<double>()) smallest = std::min(smallest, v);
      return false;
    };
    const auto r = optim::optima(f, vec({9.0, 9.0}), o, cons);
    std::cout << std::left << std::setw(18) << name << std::setw(6) << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration
              << std::scientific << std::setprecision(2) << std::setw(24) << smallest << std::fixed << std::setprecision(6)
              << r.evaluation.value.template item<double>() << "\n";
  }
  return 0;
}
