<!-- generated from the_ten_stall_categories.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The ten stall categories

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


The exit flag is coarse. The stall category says what went wrong underneath. There are ten, and this program triggers each. The scenario text describes the cause, and the line below it shows the category name and the flag it came with.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <string>
#include <vector>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

void report(const std::string& scenario, const optim::OptimaState& r) {
  std::cout << scenario << "\n    stall=" << optim::optima_stall_category_name(r.output.stall_category)
            << "  flag=" << static_cast<int>(r.exit_flag) << "\n";
}

int main() {
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });
  const auto impossible_inequality = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });
  const auto impossible_equality = optim::constraints().nl_eq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) + 1.0; });

  optim::OptimaOptions interior;
  interior.algorithm = optim::OptimaAlgorithm::InternalProtocol;

  report("a normal solve", optim::optima(objective, vec({0.0, 0.0})));

  optim::OptimaOptions hook;
  hook.progress_hook = [](const optim::OptimaProgressSnapshot&) { return true; };
  report("the progress hook asks to stop", optim::optima(objective, vec({0.0, 0.0}), hook));

  report("the objective returns a vector",
         optim::optima([](const optim::Trace& x) { return optim::square(x); }, vec({1.0, 1.0})));

  report("the objective is not finite at the start",
         optim::optima([](const optim::Trace& x) { return optim::sum(optim::log(x)); }, vec({-1.0, 1.0})));

  optim::OptimaOptions merit;
  merit.algorithm = optim::OptimaAlgorithm::MeritMethod;
  report("merit method on an impossible inequality", optim::optima(objective, vec({0.0, 0.0}), merit, impossible_inequality));

  optim::OptimaOptions no_step = interior;
  no_step.internal_protocol.fraction_to_boundary = 0.0;
  report("no step is allowed toward the boundary", optim::optima(objective, vec({0.0, 0.0}), no_step, disc));

  report("sequential QP cannot restore feasibility", optim::optima(objective, vec({0.0, 0.0}), {}, impossible_inequality));

  report("interior method stalls on an infeasible equality",
         optim::optima(objective, vec({1.0, 1.0}), interior, impossible_equality));

  optim::OptimaOptions phase = interior;
  phase.internal_protocol.enable_feasibility_phase = true;
  report("feasibility phase cannot reduce the violation", optim::optima(objective, vec({1.0, 1.0}), phase, impossible_equality));

  optim::OptimaOptions short_run = interior;
  short_run.internal_protocol.max_iterations = 2;
  report("interior method stops with barrier complementarity unresolved",
         optim::optima(objective, vec({0.1, 0.1}), short_run, disc));

  optim::OptimaOptions one_iteration;
  one_iteration.sequential_qp.max_iterations = 1;
  report("the iteration budget is spent", optim::optima(objective, vec({0.0, 0.0}), one_iteration, disc));
  return 0;
}
```

Output

```text
a normal solve
    stall=none  flag=1
the progress hook asks to stop
    stall=user-stop  flag=-1
the objective returns a vector
    stall=invalid-callback-contract  flag=-5
the objective is not finite at the start
    stall=nonfinite-callback-region  flag=-4
merit method on an impossible inequality
    stall=search-direction-collapse  flag=-6
no step is allowed toward the boundary
    stall=boundary-limited  flag=-6
sequential QP cannot restore feasibility
    stall=recovery-failure  flag=-6
interior method stalls on an infeasible equality
    stall=infeasible-stationary  flag=-7
feasibility phase cannot reduce the violation
    stall=feasibility-failure  flag=-6
interior method stops with barrier complementarity unresolved
    stall=complementarity-stagnation  flag=0
the iteration budget is spent
    stall=iteration-limit  flag=0
```
| Category | What it means |
|---|---|
| `none` | The run ended healthily. |
| `user-stop` | The progress hook asked to stop. |
| `invalid-callback-contract` | A callback returned the wrong shape or threw. |
| `nonfinite-callback-region` | A callback returned `NaN` or infinity. |
| `search-direction-collapse` | The line search could not find an acceptable step. |
| `boundary-limited` | No step was possible toward a bound or the linearized constraints. |
| `infeasible-stationary` | The constraint violation is at a stationary point that is still infeasible. |
| `complementarity-stagnation` | The interior method hit its budget with the barrier complementarity still large against stationarity. |
| `feasibility-failure` | The feasibility phase could not reduce the violation. |
| `recovery-failure` | The restoration or recovery step failed. |
| `iteration-limit` | The iteration budget was spent. |

`boundary-limited` needs an unusual setting to appear: here the interior method's `fraction_to_boundary = 0` forbids any step toward a constraint boundary. It is the category to look for when a run on a boxed problem refuses to move.
