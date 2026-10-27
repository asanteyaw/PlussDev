// What each algorithm does when the objective returns a number that is not finite. Three situations: the
// objective is NaN at the start, the objective is NaN in a region the iterates wander into, and the
// objective is +infinity beyond a wall. The first is a failure by design, because no algorithm can recover
// from a start that has no value; the other two test whether a solver backs off and carries on.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace optim = pluss::subsp::math::optim;
using Series = pluss::Series;
const auto f64 = pluss::ScalarType::Float64;

// f(x) = 100 (x - 3)^2 for x > bound, otherwise `outside` (NaN or +infinity), with the exact gradient.
optim::ConstrainedProblem wall(double bound, double outside) {
  optim::ConstrainedProblem problem;
  problem.objective.analytic = [bound, outside](const optim::ParameterPack<Series>& p)
      -> std::pair<Series, optim::ParameterPack<Series>> {
    const double x = as_f64(p.at("x"));
    if (!(x > bound)) {
      return {pluss::make_series(outside).to(f64),
              optim::ParameterPack<Series>({{"x", pluss::make_series(outside).to(f64).reshape({1})}})};
    }
    return {pluss::make_series(100.0 * (x - 3.0) * (x - 3.0)).to(f64),
            optim::ParameterPack<Series>({{"x", pluss::make_series(200.0 * (x - 3.0)).to(f64).reshape({1})}})};
  };
  return problem;
}

int main() {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  auto start = [&](double x) { return optim::pack_scalars({{"x", pluss::make_series(x).to(f64)}}); };

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};
  struct Case { const char* label; optim::ConstrainedProblem problem; double x0; };
  const std::vector<Case> cases = {{"NaN at the start", wall(100.0, nan), 4.0},
                                   {"NaN region below x = 2.5", wall(2.5, nan), 4.0},
                                   {"+inf wall below x = 2.5", wall(2.5, inf), 4.0}};

  std::cout << std::fixed << std::setprecision(4);
  for (const auto& c : cases) {
    std::cout << c.label << "\n";
    for (const auto& row : rows) {
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      const auto r = optim::optima(c.problem, start(c.x0), options);
      std::cout << "  " << std::left << std::setw(17) << row.label << "flag " << std::setw(3)
                << static_cast<int>(r.exit_flag) << "x = " << as_f64(r.params.at("x")) << "\n";
    }
  }
  return 0;
}
