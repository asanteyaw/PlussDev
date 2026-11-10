<!-- generated from to_string.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::to_string`

Returns the name of a solver kind or a status as text.

## Signature

```cpp
std::string dfo::to_string(dfo::DfoSolverKind solver);
std::string dfo::to_string(dfo::DfoStatus status);
```

## What It Does

Two overloads, one per enumeration, both returning the enumerator's own name. The names are what the examples print and what a log or a table wants.

## Parameters

`dfo::DfoSolverKind solver`
: The kind to name.

`dfo::DfoStatus status`
: The status to name.

## Returns

The enumerator name, for example `"NelderMead"` or `"Converged"`.

## Practical Examples

### Example 1

A solver kind.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln(dfo::to_string(dfo::DfoSolverKind::Auto), dfo::to_string(dfo::DfoSolverKind::LtMads));
}
```

Output

```text
Auto LtMads
```

### Example 2

A status.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln(dfo::to_string(dfo::DfoStatus::Converged), dfo::to_string(dfo::DfoStatus::InfeasibleStationary));
}
```

Output

```text
Converged InfeasibleStationary
```

### Example 3

In a report. A result prints the solver that ran and how it ended.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto f = [](const Series &x) { return (x - 3.0).square().sum(); };   // minimum 0 at (3, 3)

  const dfo::DfoResult r = dfo::solve_dfo(f, pluss::series::make({0.0, 0.0}));
  out.writeln("solver =", dfo::to_string(r.selected_solver), ", status =", dfo::to_string(r.status));
}
```

Output

```text
solver = NelderMead , status = Converged
```

## See Also

- [`DfoSolverKind`](DfoSolverKind.md): the kinds
- [`DfoStatus`](DfoStatus.md): the statuses

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="solve_dfo.html">
    <span class="label">Previous</span>
    <span class="title">solve_dfo</span>
  </a>
  <a class="next-link" href="to_string_BlackboxSolverStatus.html">
    <span class="label">Next</span>
    <span class="title">to_string(BlackboxSolverStatus)</span>
  </a>
</div>
<!-- apiref:nav:end -->
