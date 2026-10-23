<!-- generated from where_you_start_decides_where_you_end.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Where you start decides where you end

[Back to Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)


A nonconvex objective has several local minima, and each algorithm falls into one of them. This objective, `(x^2 - 1)^2 + 0.3 x`, has a deep minimum near `-1.04` and a shallow one near `0.96`. The table shows the point each algorithm reports from four starts.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto one = [&](double x) { return optim::pack_scalars({{"x", scalar(x)}}); };

  auto objective = [](const Pack& p) {
    return optim::square(optim::square(p.at("x")) - 1.0) + 0.3 * p.at("x");
  };
  const auto box = optim::constraints().lbs(one(-2.0)).ubs(one(2.0));

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};
  const std::vector<double> starts = {-1.5, -0.3, 0.3, 1.5};

  int converged = 0;
  std::cout << std::fixed << std::setprecision(3);
  std::cout << std::left << std::setw(18) << "start";
  for (double s : starts) std::cout << std::right << std::setw(9) << s;
  std::cout << "\n";
  for (const auto& row : rows) {
    std::cout << std::left << std::setw(18) << row.label;
    for (double s : starts) {
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      const auto r = optim::optima(objective, one(s), options, box);
      std::cout << std::right << std::setw(9) << r.params.at("x").item<double>();
      if (r.converged) ++converged;
    }
    std::cout << "\n";
  }
  std::cout << "runs that exited with flag Converged: " << converged << " of 16\n";
  return 0;
}
```

Output

```text
start                -1.500   -0.300    0.300    1.500
MeritMethod          -1.036   -1.036    0.960    0.960
SequentialQP          0.960   -1.036    0.960   -1.036
WorkingSet            0.960   -1.036    0.960   -1.036
InternalProtocol      0.960   -1.036    0.960   -1.036
runs that exited with flag Converged: 16 of 16
```
The last line confirms that all 16 runs exit with flag `Converged`. The merit method stays where it starts: the two left starts end at `-1.036`, the two right starts at `0.960`. The three quadratic-model methods jump across the barrier between the wells on the far starts, so the two outer starts, `-1.5` and `1.5`, each end up in the other well. The deep minimum at `-1.036` is reached from `-0.3` and from `1.5` by those three methods, and the shallow minimum from `-1.5` and `0.3`. There is no rule that a start close to a minimum reaches it, so a model with several minima needs several starts, and the answer to compare across them is the objective value, never the exit flag.
