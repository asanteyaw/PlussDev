<!-- generated from reports_detailed_summary.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::reports::detailed_summary`

Writes a multi-line summary of a result, one `name=value` per line.

## Signature

```cpp
std::ostream &optim::reports::detailed_summary(std::ostream &os, const optim::OptimaState &result);
```

## What It Does

Writes every field of `result.output` one per line: algorithm, exit flag, iterations, evaluations, constraint violation, first-order optimality, step size, backend parameter, complementarity, Krylov iterations, best-feasible bookkeeping, the stall category with its measure and reference, and the message.

## Parameters

`os`
: The stream.

`result`
: The state.

## Returns

`os`.

## Practical Examples

### Example 1

A full summary.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), {}, disc);
  optim::reports::detailed_summary(std::cout, result) << "\n";
}
```

Output

```text
algorithm=sqp
exit=1
iterations=4
evaluations=8
constraint_violation=0
first_order_optimality=2.53047e-08
step_size=1
backend_parameter=1.36967
complementarity=0
krylov_iterations=0
best_feasible_available=true
returned_best_feasible=true
stall=none
stall_measure=0
stall_reference=0
message=optimality and feasibility tolerances satisfied
```

### Example 2

The interior algorithm: `complementarity` and `backend_parameter` carry the barrier information.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  optim::reports::detailed_summary(std::cout, result) << "\n";
}
```

Output

```text
algorithm=internal-protocol
exit=1
iterations=7
evaluations=13
constraint_violation=0
first_order_optimality=5.75624e-08
step_size=0.995547
backend_parameter=1e-08
complementarity=3.10253e-07
krylov_iterations=0
best_feasible_available=true
returned_best_feasible=true
stall=none
stall_measure=0
stall_reference=0
message=barrier optimality and feasibility tolerances satisfied
```

### Example 3

A run cut short shows the stall diagnosis.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const pluss::Series target = pluss::series::make({1.0, 2.0});
  const auto objective = [&](const pluss::Series &x) { return (x - target).square().sum(); };
  const auto disc = optim::constraints().nl_ineq([](const Pack &p) { return p.at("x").square().sum() - 1.0; });

  optim::OptimaOptions options;
  options.sequential_qp.max_iterations = 1;
  const optim::OptimaState result = optim::optima(objective, pluss::series::make({0.0, 0.0}), options, disc);
  optim::reports::detailed_summary(std::cout, result) << "\n";
}
```

Output

```text
algorithm=sqp
exit=0
iterations=1
evaluations=5
constraint_violation=0.25
first_order_optimality=4.22769e-09
step_size=0.25
backend_parameter=5
complementarity=0
krylov_iterations=0
best_feasible_available=true
returned_best_feasible=false
stall=iteration-limit
stall_measure=4.22769e-09
stall_reference=0.25
message=maximum iterations reached best feasible point retained separately from the returned iterate. diagnostic=iteration-limit measure=4.228e-09 reference=2.500e-01.
```

## See Also

- [`print_summary`](print_summary.md): one line

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reports_compact_summary.html">
    <span class="label">Previous</span>
    <span class="title">compact_summary</span>
  </a>
  <a class="next-link" href="reports_iter.html">
    <span class="label">Next</span>
    <span class="title">iter</span>
  </a>
</div>
<!-- apiref:nav:end -->
