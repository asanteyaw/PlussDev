<!-- generated from restoration_the_last_resort.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Restoration, the last resort

[Back to Sequential QP in Depth](../optima_04_sequential_qp.md)


Restoration runs only after the line search has failed twice in a row, so the program starves the line search on purpose to reach it.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  const auto start = optim::pack_scalars({{"x", scalar(-2.0)}, {"y", scalar(-1.2)}});
  auto f = [](const Pack& p) { return 2.0 * (optim::square(p.at("x")) + optim::square(p.at("y")) - 1.0) - p.at("x"); };
  const auto cons = optim::constraints().nl_eq(
      [](const Pack& p) { return optim::square(p.at("x")) + optim::square(p.at("y")) - 1.0; });

  std::cout << std::left << std::setw(26) << "max_line_search_iterations" << std::setw(13) << "restoration" << std::setw(6)
            << "flag" << std::setw(7) << "iters" << std::setw(7) << "evals" << "message\n";
  for (std::int64_t budget : {1, 3, 20}) {
    for (bool on : {true, false}) {
      optim::OptimaOptions o;
      o.algorithm = optim::OptimaAlgorithm::SequentialQP;
      o.sequential_qp.max_line_search_iterations = budget;
      o.sequential_qp.max_second_order_corrections = 0;
      o.sequential_qp.enable_restoration = on;
      const auto r = optim::optima(f, start, o, cons);
      std::cout << std::left << std::setw(26) << budget << std::setw(13) << (on ? "enabled" : "disabled") << std::setw(6)
                << static_cast<int>(r.exit_flag) << std::setw(7) << r.iteration << std::setw(7) << r.function_evaluations
                << r.message.substr(0, 56) << "\n";
    }
  }
  return 0;
}
```

Output

```text
max_line_search_iterationsrestoration  flag  iters  evals  message
1                         enabled      -6    3      21     line search failed to produce an acceptable SQP step dia
1                         disabled     -6    2      6      line search failed to produce an acceptable SQP step dia
3                         enabled      1     9      18     optimality and feasibility tolerances satisfied
3                         disabled     1     9      18     optimality and feasibility tolerances satisfied
20                        enabled      1     12     22     optimality and feasibility tolerances satisfied
20                        disabled     1     12     22     optimality and feasibility tolerances satisfied
```
With a budget of 3 or more trial steps the two settings are identical, so restoration never ran. With a budget of 1, restoration is entered, spends 21 evaluations against 6, and still ends in failure. Restoration is a safety net for a line search that has an adequate budget but meets a bad region; it cannot repair a line search that has been starved. If you see the failure message, raise `max_line_search_iterations` first.
