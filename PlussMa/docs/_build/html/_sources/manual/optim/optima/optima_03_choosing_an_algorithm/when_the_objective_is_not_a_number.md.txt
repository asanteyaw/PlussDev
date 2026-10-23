<!-- generated from when_the_objective_is_not_a_number.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# When the objective is not a number

[Back to Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)


Real models return `NaN` or infinity in regions the optimizer should never have visited: a log of a negative number, a variance that has gone negative. The three cases below are the ones that matter: a start with no value, a region of `NaN`, and a wall of `+infinity`.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace optim = pluss::optim;
using Series = pluss::Series;
const auto f64 = pluss::ScalarType::Float64;

// f(x) = 100 (x - 3)^2 for x > bound, otherwise `outside` (NaN or +infinity), with the exact gradient.
optim::ConstrainedProblem wall(double bound, double outside) {
  optim::ConstrainedProblem problem;
  problem.objective.analytic = [bound, outside](const optim::ParameterPack<Series>& p)
      -> std::pair<Series, optim::ParameterPack<Series>> {
    const double x = p.at("x").item<double>();
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
                << static_cast<int>(r.exit_flag) << "x = " << r.params.at("x").item<double>() << "\n";
    }
  }
  return 0;
}
```

Output

```text
NaN at the start
  MeritMethod      flag -4 x = 4.0000
  SequentialQP     flag -4 x = 4.0000
  WorkingSet       flag -4 x = 4.0000
  InternalProtocol flag -4 x = 4.0000
NaN region below x = 2.5
  MeritMethod      flag 1  x = 3.0000
  SequentialQP     flag 1  x = 3.0000
  WorkingSet       flag 1  x = 3.0000
  InternalProtocol flag 1  x = 3.0000
+inf wall below x = 2.5
  MeritMethod      flag 1  x = 3.0000
  SequentialQP     flag 1  x = 3.0000
  WorkingSet       flag 1  x = 3.0000
  InternalProtocol flag 1  x = 3.0000
```
A start without a value is a failure for all four, flag `-4`, and the start is returned untouched: no algorithm can step from a point it cannot evaluate. In the other two cases every algorithm backs off the bad region and converges to `x = 3`. This is the behaviour to rely on when an optimizer wanders into a bad region. It does not make a start without a value usable.
