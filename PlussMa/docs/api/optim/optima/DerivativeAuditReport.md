<!-- generated from DerivativeAuditReport.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::DerivativeAuditReport`

The outcome of `audit_optima_derivatives`: one entry per function checked, and an overall verdict.

`objective`, `nonlinear_equalities` and `nonlinear_inequalities` are each a `DerivativeAuditEntry` with `available` (both forms were present), `passed`, the value differences `value_abs_diff`, `value_rel_diff`, the derivative differences `derivative_abs_diff`, `derivative_rel_diff`, and a `message`. `passed` is true when every available entry passed.

## Fields

### `objective`

`optim::DerivativeAuditEntry`, default `unavailable`.

Audit of the objective gradient.

### `nonlinear_equalities`

`optim::DerivativeAuditEntry`, default `unavailable`.

Audit of the equality Jacobian.

### `nonlinear_inequalities`

`optim::DerivativeAuditEntry`, default `unavailable`.

Audit of the inequality Jacobian.

### `passed`

`bool`, default `true`.

Every available entry passed.

## Guidance

Returned by `audit_optima_derivatives`.

## Practical Examples

### Example 1

Reading a report.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const Pack at = optim::pack_series(pluss::series::make({1.0, 2.0}));
  const optim::ConstrainedProblem problem = optim::problem()
      .objective_analytic([](const Pack &p) { return std::pair{p.at("x").square().sum(), optim::pack_series(2.0 * p.at("x"))}; })
      .objective([](const Pack &p) { return p.at("x").square().sum(); }).build();
  const optim::DerivativeAuditReport report = optim::audit_optima_derivatives(problem, at);
  out.writeln("overall =", report.passed, ", objective available =", report.objective.available, ", passed =", report.objective.passed);
  out.writeln("value difference =", report.objective.value_abs_diff, ", derivative difference =", report.objective.derivative_abs_diff);
}
```

Output

```text
overall = true , objective available = true , passed = true
value difference = 0.00000000 , derivative difference = 0.00000000
```

### Example 2

Entries that could not be compared.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const optim::ConstrainedProblem problem = optim::problem().objective([](const Pack &p) { return p.at("x").square().sum(); }).build();
  const optim::DerivativeAuditReport report = optim::audit_optima_derivatives(problem, optim::pack_series(pluss::series::make({1.0})));
  out.writeln("objective available =", report.objective.available, ", equalities available =", report.nonlinear_equalities.available);
}
```

Output

```text
objective available = false , equalities available = false
```

### Example 3

A failed entry carries a message.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<pluss::Series>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const Pack at = optim::pack_series(pluss::series::make({1.0, 2.0}));
  const optim::ConstrainedProblem problem = optim::problem()
      .objective_analytic([](const Pack &p) { return std::pair{p.at("x").square().sum(), optim::pack_series(3.0 * p.at("x"))}; })
      .objective([](const Pack &p) { return p.at("x").square().sum(); }).build();
  const optim::DerivativeAuditReport report = optim::audit_optima_derivatives(problem, at);
  out.writeln("passed =", report.objective.passed, ", message =", report.objective.message);
}
```

Output

```text
passed = false , message = objective analytic vs autodiff
```

## See Also

- [`audit_optima_derivatives`](audit_optima_derivatives.md): produces it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ConstraintMultipliers.html">
    <span class="label">Previous</span>
    <span class="title">ConstraintMultipliers</span>
  </a>
  <a class="next-link" href="InternalProtocolOptions.html">
    <span class="label">Next</span>
    <span class="title">InternalProtocolOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
