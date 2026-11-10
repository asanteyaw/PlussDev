<!-- generated from coarsest_cells.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 774ab8594007 -->

# `multigrid::Hierarchy::coarsest_cells`

[Back to `multigrid::Hierarchy`](../Hierarchy.md)

The number of cells on the coarsest level.

## Signature

```cpp
[[nodiscard]] std::int64_t multigrid::Hierarchy::coarsest_cells() const;
```

## What It Does

The product of the cell counts of all axes on the last level. When it is at most `Options::dense_limit` the level is solved exactly; otherwise it is smoothed. Comparing it with `Options::coarsest_cells` tells you why coarsening stopped: at or below the target, it reached it; above, an extent became odd or `max_levels` was reached first.

## Returns

The coarsest cell count (per lane).

## Practical Examples

### Example 1

The coarsest size follows the coarsening target.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace mg = pluss::subsp::math::multigrid;
namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  for (const std::int64_t target : {4, 64, 512}) {
    mg::Options o;
    o.coarsest_cells = target;
    const mg::Hierarchy h({.axes = {{1024, 1.0, mg::Edge::Dirichlet, mg::Edge::Dirichlet}}}, pluss::series::ones({1024}, pluss::ScalarType::Float64), o);
    out.writeln("target", target, ": coarsest =", h.coarsest_cells(), ", levels =", h.levels());
  }
}
```

Output

```text
target 4 : coarsest = 4 , levels = 9
target 64 : coarsest = 64 , levels = 5
target 512 : coarsest = 512 , levels = 2
```

### Example 2

An odd extent stops coarsening above the target. 1000 cells stop at 125, though the target is 64.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace mg = pluss::subsp::math::multigrid;
namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const mg::Hierarchy h({.axes = {{1000, 1.0, mg::Edge::Dirichlet, mg::Edge::Dirichlet}}}, pluss::series::ones({1000}, pluss::ScalarType::Float64));
  out.writeln("coarsest =", h.coarsest_cells());
}
```

Output

```text
coarsest = 125
```

### Example 3

The product over axes in 3-D.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace mg = pluss::subsp::math::multigrid;
namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(0));
  const mg::Hierarchy h({.axes = {{32, 1.0, mg::Edge::Dirichlet, mg::Edge::Dirichlet}, {32, 1.0, mg::Edge::Dirichlet, mg::Edge::Dirichlet}, {32, 1.0, mg::Edge::Dirichlet, mg::Edge::Dirichlet}}}, pluss::series::ones({32, 32, 32}, pluss::ScalarType::Float64));
  out.writeln("levels =", h.levels(), ", coarsest cells =", h.coarsest_cells());
}
```

Output

```text
levels = 4 , coarsest cells = 64
```

## See Also

- [`Hierarchy`](../Hierarchy.md): the class
- [`levels`](levels.md): the depth

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply.html">
    <span class="label">Previous</span>
    <span class="title">apply</span>
  </a>
  <a class="next-link" href="levels.html">
    <span class="label">Next</span>
    <span class="title">levels</span>
  </a>
</div>
<!-- apiref:nav:end -->
