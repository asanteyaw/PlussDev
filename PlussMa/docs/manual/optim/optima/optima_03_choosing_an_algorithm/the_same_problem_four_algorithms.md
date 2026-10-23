<!-- generated from the_same_problem_four_algorithms.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The same problem, four algorithms

[Back to Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)


The first program solves one problem with all four algorithms. It has a nonlinear equality, a nonlinear inequality and bounds, and it is started on the boundary of the inequality and on two bounds, which is a deliberately awkward start. The table has the same columns as the report from chapter 2, so every number can be compared across rows.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };

  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto four = [&](double a, double b, double c, double d) {
    return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}, {"c", scalar(c)}, {"d", scalar(d)}});
  };

  // minimize a*d*(a + b + c) + c
  //   subject to a*b*c*d >= 25, a^2 + b^2 + c^2 + d^2 = 40, 1 <= each <= 5
  auto objective = [](const Pack& p) {
    return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
  };
  const auto constraints =
      optim::constraints()
          .nl_ineq([](const Pack& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
          .nl_eq([](const Pack& p) {
            return optim::square(p.at("a")) + optim::square(p.at("b")) + optim::square(p.at("c")) +
                   optim::square(p.at("d")) - 40.0;
          })
          .lbs(four(1, 1, 1, 1))
          .ubs(four(5, 5, 5, 5));

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  optim::OptimaState merit_result;
  std::cout << std::left << std::setw(17) << "algorithm" << std::setw(6) << "flag" << std::setw(7) << "iters"
            << std::setw(7) << "evals" << std::setw(14) << "objective" << "violation\n";
  for (const auto& row : rows) {
    optim::OptimaOptions options;
    options.algorithm = row.algorithm;
    const auto r = optim::optima(objective, four(1, 5, 5, 1), options, constraints);
    if (row.algorithm == optim::OptimaAlgorithm::MeritMethod) merit_result = r;
    std::cout << std::left << std::setw(17) << row.label << std::setw(6) << static_cast<int>(r.exit_flag)
              << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations << std::fixed
              << std::setprecision(6) << std::setw(14) << r.evaluation.value.item<double>() << std::scientific
              << std::setprecision(2) << r.evaluation.max_constraint_violation << std::defaultfloat << "\n";
  }
  std::cout << std::fixed << std::setprecision(4) << "\nMeritMethod ended at a = "
            << merit_result.params.at("a").item<double>() << ", b = " << merit_result.params.at("b").item<double>()
            << ", c = " << merit_result.params.at("c").item<double>() << ", d = "
            << merit_result.params.at("d").item<double>() << " (a is at its lower bound 1, b at its upper bound 5)\n";
  return 0;
}
```

Output

```text
algorithm        flag  iters  evals  objective     violation
MeritMethod      1     66     180    27.146428     1.28e-08
SequentialQP     1     4      5      17.014017     8.24e-08
WorkingSet       1     4      6      17.014017     8.24e-08
InternalProtocol 1     8      12     17.014018     8.84e-08

MeritMethod ended at a = 1.0000, b = 5.0000, c = 1.4495, d = 3.4495 (a is at its lower bound 1, b at its upper bound 5)
```
All four exit with flag `1`, but they do not agree. Sequential QP, the working-set method and the interior method reach `17.0140`, the best known value for this problem, in 4, 4 and 8 iterations. The merit method needs 66 iterations and ends at `27.1464`. That second value is not a failure: the last line shows it is a different point, with `a` on its lower bound and `b` on its upper bound, and the solver certifies it as a local solution. A flag of `1` means "a local solution", and this table is the reason to remember it. The same row-by-row difference, fewer iterations for the quadratic-model methods, holds across the rest of the chapter.
