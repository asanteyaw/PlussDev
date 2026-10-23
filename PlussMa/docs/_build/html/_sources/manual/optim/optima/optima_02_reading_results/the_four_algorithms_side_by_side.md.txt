<!-- generated from the_four_algorithms_side_by_side.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The four algorithms side by side

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


Choosing an algorithm is the subject of the next chapter. For now, the first program shows that all four share one result type, so every report in this chapter works the same for each. It also shows how little they differ in what they return on an easy problem, and how much in the work they do to get there.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  const auto target = optim::trace(vec({2.0, 1.0}), false);
  auto objective = [&](const optim::Trace& x) { return optim::sum(optim::square(x - target)); };
  const auto constraints = optim::constraints().ubs(vec({1.5, 0.8}));

  const std::vector<std::pair<std::string, optim::OptimaAlgorithm>> algorithms = {
      {"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
      {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
      {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
      {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  std::cout << std::fixed << std::setprecision(4);
  for (const auto& [name, algorithm] : algorithms) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    const auto r = optim::optima(objective, vec({0.5, 0.5}), options, constraints);
    const auto x = r.params.at("x").to_vec<double>();
    std::cout << std::left << std::setw(17) << name << std::setw(24) << ("label=" + r.output.algorithm) << std::right
              << "x=[" << x[0] << ", " << x[1] << "]  flag=" << std::setw(2) << static_cast<int>(r.exit_flag)
              << "  iterations=" << std::setw(3) << r.iteration << "  evaluations=" << std::setw(3)
              << r.function_evaluations << "\n";
  }
  return 0;
}
```

Output

```text
MeritMethod      label=merit-method      x=[1.5000, 0.8000]  flag= 1  iterations=  2  evaluations=  3
SequentialQP     label=sqp               x=[1.5000, 0.8000]  flag= 1  iterations=  1  evaluations=  2
WorkingSet       label=working-set       x=[1.5000, 0.8000]  flag= 1  iterations=  1  evaluations=  2
InternalProtocol label=internal-protocol x=[1.5000, 0.8000]  flag= 1  iterations=  5  evaluations=  8
```
All four agree on the answer, the corner `(1.5, 0.8)`, and all four exit with `Converged`. They differ in effort: the merit method needs two iterations and three evaluations because it handles bounds by projection, while sequential QP and the working-set method need a single iteration, because their first quadratic subproblem already identifies the corner, and the interior method takes five iterations and eight evaluations, since a barrier method approaches a bound from inside. `output.algorithm` carries each method's label.
