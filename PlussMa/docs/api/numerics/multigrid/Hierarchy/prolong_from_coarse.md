<!-- generated from prolong_from_coarse.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a9c7c6f629a5 -->

# `multigrid::Hierarchy::prolong_from_coarse`

[Back to `multigrid::Hierarchy`](../Hierarchy.md)

Transfers a field from level `l + 1` up to the finer level `l`.

## Signature

```cpp
[[nodiscard]] pluss::Series multigrid::Hierarchy::prolong_from_coarse(std::size_t l, pluss::Series c) const;
```

## What It Does

Tensor-product linear interpolation: each fine cell takes `¾` of the coarse cell it lies in and `¼` of the nearer neighbour, with wall-reflected ghosts at the ends (the neighbour beyond a Dirichlet wall is `−c`, beyond a Neumann wall `+c`). With variable coefficients the weights are adjusted to the face coefficients. It is the correction transfer of the V-cycle.

## Parameters

`std::size_t l`
: The fine level, `0 ≤ l < levels() − 1`.

`pluss::Series c`
: A field on level `l + 1`.

## Returns

The field on level `l`.

## Errors

- A level out of range, or a field of the wrong shape, throws.

## Practical Examples

### Example 1

Interpolation of a smooth field. A linear ramp on the coarse grid prolongs to a nearly linear ramp on the fine grid (exact away from the walls).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace mg = pluss::subsp::math::multigrid;
namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mg::Options o;
  o.coarsest_cells = 2;
  const Series c = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  const mg::Hierarchy h({.axes = {{8, 1.0, mg::Edge::Neumann, mg::Edge::Neumann}}}, pluss::series::ones({8}, pluss::ScalarType::Float64), o);
  out.writeln(h.prolong_from_coarse(0, c).items<double>());
}
```

Output

```text
[1.0000, 1.2500, 1.7500, 2.2500, 2.7500, 3.2500, 3.7500, 4.0000]
```

### Example 2

The boundary reflection. A constant coarse field prolongs to a constant for Neumann ends, but at Dirichlet walls the end cells are halved, because the neighbour beyond the wall is `−c`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace mg = pluss::subsp::math::multigrid;
namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mg::Options o;
  o.coarsest_cells = 2;
  const Series c = pluss::series::ones({4}, pluss::ScalarType::Float64);
  for (const mg::Edge edge : {mg::Edge::Neumann, mg::Edge::Dirichlet}) {
    const mg::Hierarchy h({.axes = {{8, 1.0, edge, edge}}}, pluss::series::ones({8}, pluss::ScalarType::Float64), o);
    out.writeln(static_cast<int>(edge), ":", h.prolong_from_coarse(0, c).items<double>());
  }
}
```

Output

```text
2 : [1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
1 : [0.5000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 0.5000]
```

### Example 3

Round trip. Restricting after prolonging gives back the coarse field, a smoothing of it (a weighted average with the neighbours).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace mg = pluss::subsp::math::multigrid;
namespace krylov = pluss::subsp::math::krylov;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  mg::Options o;
  o.coarsest_cells = 2;
  const Series c = pluss::make_series({1.0, 2.0, 4.0, 8.0});
  const mg::Hierarchy h({.axes = {{8, 1.0, mg::Edge::Neumann, mg::Edge::Neumann}}}, pluss::series::ones({8}, pluss::ScalarType::Float64), o);
  out.writeln("coarse   :", c.items<double>());
  out.writeln("R P coarse:", h.restrict_to_coarse(0, h.prolong_from_coarse(0, c)).items<double>());
}
```

Output

```text
coarse   : [1.0000, 2.0000, 4.0000, 8.0000]
R P coarse: [1.1875, 2.1875, 4.3750, 7.2500]
```

## See Also

- [`Hierarchy`](../Hierarchy.md): the class
- [`restrict_to_coarse`](restrict_to_coarse.md): the opposite transfer

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="levels.html">
    <span class="label">Previous</span>
    <span class="title">levels</span>
  </a>
  <a class="next-link" href="restrict_to_coarse.html">
    <span class="label">Next</span>
    <span class="title">restrict_to_coarse</span>
  </a>
</div>
<!-- apiref:nav:end -->
