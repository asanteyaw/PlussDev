<!-- generated from print_summary.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `optim::print_summary`

Writes a one-line summary of an Optima result to a stream.

## Signature

```cpp
std::ostream &optim::print_summary(std::ostream &os, const optim::OptimaState &result);
```

## What It Does

Writes `algorithm`, `exit` (the integer flag), `iterations`, `evaluations`, `objective`, `violation`, `first_order`, `backend_parameter`, `complementarity`, `stall` and `message` as `name=value` pairs on one line, and returns the stream. [`reports::compact_summary`](reports_compact_summary.md) is the same function under the reports namespace.

## Parameters

`os`
: The stream.

`result`
: The state to summarise.

## Returns

`os`.

## Practical Examples

### Example 1

A one-line summary.

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
  optim::print_summary(std::cout, result) << "\n";
}
```

Output

```text
algorithm=sqp exit=1 iterations=4 evaluations=8 objective=1.52786 violation=0 first_order=2.53047e-08 backend_parameter=1.36967 complementarity=0 stall=none message=optimality and feasibility tolerances satisfied
```

### Example 2

Logging a sweep, one line per start.

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

  for (const double s : {0.0, -1.0, 2.0}) {
    optim::print_summary(std::cout, optim::optima(objective, pluss::series::make({s, s}), {}, disc)) << "\n";
  }
}
```

Output

```text
algorithm=sqp exit=1 iterations=4 evaluations=8 objective=1.52786 violation=0 first_order=2.53047e-08 backend_parameter=1.36967 complementarity=0 stall=none message=optimality and feasibility tolerances satisfied
algorithm=sqp exit=1 iterations=6 evaluations=13 objective=1.52786 violation=5.10724e-09 first_order=1.26386e-05 backend_parameter=1.36967 complementarity=0 stall=none message=predicted objective improvement below tolerance at a feasible point
algorithm=sqp exit=1 iterations=8 evaluations=9 objective=1.52786 violation=4.45974e-09 first_order=3.51968e-06 backend_parameter=1.36967 complementarity=0 stall=none message=predicted objective improvement below tolerance at a feasible point
```

### Example 3

To a string stream, for storing with other results.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>

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
  std::ostringstream text;
  optim::print_summary(text, result);
  out.writeln("length of summary =", text.str().size() > 0);
}
```

Output

```text
length of summary = true
```

## See Also

- [`reports::detailed_summary`](reports_detailed_summary.md): one line per field

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="presets_sequential_qp.html">
    <span class="label">Previous</span>
    <span class="title">sequential_qp</span>
  </a>
  <a class="next-link" href="problem.html">
    <span class="label">Next</span>
    <span class="title">problem</span>
  </a>
</div>
<!-- apiref:nav:end -->
