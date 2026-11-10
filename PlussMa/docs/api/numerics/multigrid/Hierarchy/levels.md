<!-- generated from levels.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 92762a5bfe08 -->

# `multigrid::Hierarchy::levels`

[Back to `multigrid::Hierarchy`](../Hierarchy.md)

The number of levels in the hierarchy, finest included.

## Signature

```cpp
[[nodiscard]] std::size_t multigrid::Hierarchy::levels() const noexcept;
```

## What It Does

The finest grid is level 0; each coarsening halves every axis, until the cell count is at most `Options::coarsest_cells`, an extent is odd, or `Options::max_levels` is reached. The count is the depth of the V-cycle, and it is what makes the cycle cost proportional to the number of cells: each level has a `2⁻ᵈ` fraction of the cells of the one above.

## Returns

The number of levels, at least 1.

## Practical Examples

### Example 1

Depth against size. Quadrupling the cells of a 1-D grid adds two levels.

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
  for (const std::int64_t n : {64, 256, 1024, 4096})
    out.writeln("cells", n, ": levels =", mg::Hierarchy({.axes = {{n, 1.0, mg::Edge::Dirichlet, mg::Edge::Dirichlet}}}, pluss::series::ones({n}, pluss::ScalarType::Float64)).levels());
}
```

Output

```text
cells 64 : levels = 1
cells 256 : levels = 3
cells 1024 : levels = 5
cells 4096 : levels = 7
```

### Example 2

An odd extent stops the coarsening. 1000 cells halve three times (to 125, which is odd); 1024 keeps going.

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
  for (const std::int64_t n : {1000, 1024}) {
    const mg::Hierarchy h({.axes = {{n, 1.0, mg::Edge::Dirichlet, mg::Edge::Dirichlet}}}, pluss::series::ones({n}, pluss::ScalarType::Float64));
    out.writeln("cells", n, ": levels =", h.levels(), ", coarsest =", h.coarsest_cells());
  }
}
```

Output

```text
cells 1000 : levels = 4 , coarsest = 125
cells 1024 : levels = 5 , coarsest = 64
```

### Example 3

Two dimensions. Every axis halves together, so cells fall by four per level.

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
  const mg::Hierarchy h({.axes = {{128, 1.0, mg::Edge::Dirichlet, mg::Edge::Dirichlet}, {128, 1.0, mg::Edge::Dirichlet, mg::Edge::Dirichlet}}}, pluss::series::ones({128, 128}, pluss::ScalarType::Float64));
  out.writeln("levels =", h.levels(), ", coarsest cells =", h.coarsest_cells());
}
```

Output

```text
levels = 5 , coarsest cells = 64
```

## See Also

- [`Hierarchy`](../Hierarchy.md): the class
- [`coarsest_cells`](coarsest_cells.md): the size of the last level

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="coarsest_cells.html">
    <span class="label">Previous</span>
    <span class="title">coarsest_cells</span>
  </a>
  <a class="next-link" href="prolong_from_coarse.html">
    <span class="label">Next</span>
    <span class="title">prolong_from_coarse</span>
  </a>
</div>
<!-- apiref:nav:end -->
