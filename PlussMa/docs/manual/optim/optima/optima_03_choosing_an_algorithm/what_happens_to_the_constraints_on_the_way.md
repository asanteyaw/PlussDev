<!-- generated from what_happens_to_the_constraints_on_the_way.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# What happens to the constraints on the way

[Back to Choosing and Tuning an Algorithm](../optima_03_choosing_an_algorithm.md)


An algorithm that is cheap overall can still be unusable if your objective is undefined where the constraints are violated. This program records the constraint violation at every iteration and counts how many iterates were infeasible, from a start inside the feasible region and from a start outside it.

```cpp
#include <pluss/ma.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto scalar = [&](double v) { return pluss::make_series(v).to(f64); };
  auto two = [&](double a, double b) { return optim::pack_scalars({{"a", scalar(a)}, {"b", scalar(b)}}); };

  // minimize (a - 3)^2 + (b - 3)^2 subject to a^2 + b^2 <= 2 (a disc around the origin)
  auto objective = [](const Pack& p) {
    return optim::square(p.at("a") - 3.0) + optim::square(p.at("b") - 3.0);
  };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::square(p.at("a")) + optim::square(p.at("b")) - 2.0; });

  struct Row { const char* label; optim::OptimaAlgorithm algorithm; };
  const std::vector<Row> rows = {{"MeritMethod", optim::OptimaAlgorithm::MeritMethod},
                                 {"SequentialQP", optim::OptimaAlgorithm::SequentialQP},
                                 {"WorkingSet", optim::OptimaAlgorithm::WorkingSet},
                                 {"InternalProtocol", optim::OptimaAlgorithm::InternalProtocol}};

  for (const auto& start : {std::pair{"inside the disc at (0, 0)", two(0, 0)},
                            std::pair{"outside the disc at (3, 3)", two(3, 3)}}) {
    std::cout << "start " << start.first << "\n";
    for (const auto& row : rows) {
      std::vector<double> violation;
      optim::OptimaOptions options;
      options.algorithm = row.algorithm;
      options.progress_hook = [&](const optim::OptimaProgressSnapshot& s) {
        if (s.stage == optim::OptimaProgressStage::Iteration)
          violation.push_back(s.evaluation.max_constraint_violation);
        return false;
      };
      const auto r = optim::optima(objective, start.second, options, disc);
      const auto infeasible = std::count_if(violation.begin(), violation.end(), [](double v) { return v > 1e-6; });
      std::cout << "  " << std::left << std::setw(17) << row.label << "iterations " << std::setw(3)
                << violation.size() << " infeasible iterates " << std::setw(3) << infeasible << " flag "
                << static_cast<int>(r.exit_flag) << "\n";
    }
  }
  return 0;
}
```

Output

```text
start inside the disc at (0, 0)
  MeritMethod      iterations 18  infeasible iterates 13  flag 1
  SequentialQP     iterations 6   infeasible iterates 4   flag 1
  WorkingSet       iterations 9   infeasible iterates 0   flag 1
  InternalProtocol iterations 6   infeasible iterates 2   flag 1
start outside the disc at (3, 3)
  MeritMethod      iterations 21  infeasible iterates 19  flag 1
  SequentialQP     iterations 6   infeasible iterates 4   flag 1
  WorkingSet       iterations 12  infeasible iterates 2   flag 1
  InternalProtocol iterations 7   infeasible iterates 6   flag 1
```
No algorithm here promises feasible iterates. The merit method is infeasible at most iterations because it trades violation against the objective until the end. Sequential QP crosses the boundary on four iterations, and the interior method on two or six, because by default each nonlinear inequality has its own slack variable (Program 7 shows what that buys and how to switch it off). The working-set method is the most conservative here: no infeasible iterate from the feasible start, two from the infeasible one. If your objective cannot be evaluated outside the feasible set, use `InternalProtocol` with `independent_nonlinear_slacks = false`, which keeps every inequality strictly satisfied, and test it on your problem rather than taking it on trust.
