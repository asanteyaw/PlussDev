# Differential operators on periodic grids

[Back to Methods 7: Grids and operators](../meth_07_grids_and_operators.md)

Fields in physics (temperature, velocity, concentration) live on grids, and the equations that govern them are written with gradients, divergences, curls and Laplacians. This page applies those operators to fields on a periodic grid, where the right edge wraps around to the left. You need it before writing any field solver, because every one of them is built from the calls here, and because the layout rules for vector fields are the first thing that goes wrong.

## How it works

A periodic grid is made with `spatial::make_periodic_grid(shape, spacing)`: one to three axes, each with at least three cells, and one spacing per axis as a length quantity. A scalar field is a Series whose **last** `d` axes match the grid; any axes before them are batch axes that every operator leaves alone. A vector field has its component axis immediately before the grid axes, `[..., component, x, y]`, and a tensor field has two such axes.

The operators are finite differences built from shifts. The first derivative along an axis is the centred difference, the second derivative is the three-point stencil:

```{math}
\partial_x u \approx \frac{u_{i+1} - u_{i-1}}{2h}, \qquad \partial_x^2 u \approx \frac{u_{i+1} - 2 u_i + u_{i-1}}{h^2},
```

both accurate to second order in `h`. `gradient` stacks the derivatives into a vector field, `divergence` sums the derivatives of the components, `curl` is the scalar in two dimensions and a vector in three, and `laplacian` sums the second derivatives. Three more are needed by the solvers: `fwd_diff` and `back_diff`, the one-sided differences whose composition is exactly the compact Laplacian, and `wide_laplacian`, the stencil `(u_{i+2} - 2 u_i + u_{i-2}) / (4 h^2)` that equals the divergence of the centred gradient. `advect(velocity, field, grid)` computes `u . grad f`. Each operator also accepts a `SeriesQuantity` and returns one whose dimension is the field's dimension divided by length (or length squared), checked at compile time.

## Derivatives and their accuracy

The simplest check of any difference operator is to apply it to a function whose derivative you know. The program differentiates `sin x` on a one-dimensional grid of 16, 32 and 64 cells over one period, and prints the largest error of `partial` (the first derivative, `cos x`) and `second_partial` (the second derivative, `-sin x`), with the ratio of each error to that on the grid with half as many cells.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;

// cell-centred coordinates of a periodic grid along one axis, broadcast over the whole grid
static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(grid.shape().size(), 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const double two_pi = 2.0 * std::numbers::pi;
  double previous_d1 = 0.0, previous_d2 = 0.0;
  for (std::int64_t n : {16, 32, 64}) {
    const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
    const auto grid = spatial::make_periodic_grid({n}, {h});
    const pluss::Series x = coordinate(grid, 0);
    const pluss::Series f = x.sin();
    const double e1 = spatial::partial(f, grid, 0).sub(x.cos()).abs().max().item<double>();              // d/dx sin x = cos x
    const double e2 = spatial::second_partial(f, grid, 0).add(f).abs().max().item<double>();             // d2/dx2 sin x = -sin x
    out.writeln(n, " cells: first-derivative error ", e1, ", second-derivative error ", e2);
    if (previous_d1 > 0.0) out.writeln("          error ratios against the coarser grid: ", previous_d1 / e1, " and ", previous_d2 / e2);
    previous_d1 = e1;
    previous_d2 = e2;
  }
}
```

Output

```text
16 cells: first-derivative error 2.50e-02, second-derivative error 1.25e-02
32 cells: first-derivative error 6.38e-03, second-derivative error 3.19e-03
          error ratios against the coarser grid: 3.92e+00 and 3.93e+00
64 cells: first-derivative error 1.60e-03, second-derivative error 8.02e-04
          error ratios against the coarser grid: 3.98e+00 and 3.98e+00
```

Doubling the number of cells divides both errors by almost exactly four (`3.92`, then `3.98`), which is second-order accuracy: the error is proportional to `h^2`. The first derivative is `2.5e-02` on 16 cells and `1.6e-03` on 64.

## Vector calculus on a two-dimensional grid

In two dimensions the operators combine, and the identities of vector calculus become checks on the code. The program takes `f = sin x cos y` on a 32-by-32 grid and computes its gradient, which has shape `[2, 32, 32]`, then checks the Laplacian two ways (the compact stencil and the divergence of the gradient), the curl of a gradient, and the divergence and curl of the rotation field `(sin y, -cos x)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(grid.shape().size(), 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const double two_pi = 2.0 * std::numbers::pi;
  const std::int64_t n = 32;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const pluss::Series x = coordinate(grid, 0), y = coordinate(grid, 1);
  const pluss::Series f = x.sin().mul(y.cos());  // f = sin x cos y on a 2-D grid

  const pluss::Series grad = spatial::gradient(f, grid);   // [2, 32, 32]: component axis first, then the grid
  out.writeln("gradient shape: ", grad.size(0), " x ", grad.size(1), " x ", grad.size(2));
  out.writeln("df/dx error ", grad.select(0, 0).sub(x.cos().mul(y.cos())).abs().max().item<double>(), ", df/dy error ", grad.select(0, 1).sub(x.sin().mul(y.sin()).neg()).abs().max().item<double>());
  out.writeln("compact laplacian error against -2 f: ", spatial::laplacian(f, grid).add(f.mul(2.0)).abs().max().item<double>());
  out.writeln("wide laplacian error against -2 f:    ", spatial::wide_laplacian(f, grid).add(f.mul(2.0)).abs().max().item<double>());
  out.writeln("divergence of the gradient, error against -2 f: ", spatial::divergence(grad, grid).add(f.mul(2.0)).abs().max().item<double>());
  out.writeln("curl of the gradient (should be zero): ", spatial::curl(grad, grid).abs().max().item<double>());
  const pluss::Series rotation = y.sin().unsqueeze(0).unite(std::vector<pluss::Series>{x.cos().neg().unsqueeze(0)}, 0);  // (sin y, -cos x) has no divergence
  out.writeln("divergence of (sin y, -cos x): ", spatial::divergence(rotation, grid).abs().max().item<double>());
  out.writeln("curl of (sin y, -cos x), error against sin x - cos y: ", spatial::curl(rotation, grid).sub(x.sin().sub(y.cos())).abs().max().item<double>());
}
```

Output

```text
gradient shape: 2 x 32 x 32
df/dx error 6.35e-03, df/dy error 6.35e-03
compact laplacian error against -2 f: 6.36e-03
wide laplacian error against -2 f:    2.53e-02
divergence of the gradient, error against -2 f: 2.53e-02
curl of the gradient (should be zero): 6.11e-16
divergence of (sin y, -cos x): 0.00e+00
curl of (sin y, -cos x), error against sin x - cos y: 1.28e-02
```

The gradient components are each within `6.4e-03` of the exact ones, and the compact Laplacian is within `6.4e-03` of `-2 f`. The divergence of the gradient equals the **wide** Laplacian (both show `2.53e-02`), a stencil with spacing `2h` and so a larger error on the same grid; the compact one is the better approximation. The curl of a gradient is zero to rounding, `6e-16`, and the rotation field has exactly zero divergence.

## Transport, units and batches

A transport term `u . grad f` is the rate at which a velocity field carries a quantity along. The program moves `f = sin x sin y` in the uniform wind `(2, 1)`, differentiates a pressure field and asserts at compile time that the result is a pressure per length, composes the one-sided differences into the Laplacian, and differentiates a batch of two fields in one call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(grid.shape().size(), 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const double two_pi = 2.0 * std::numbers::pi;
  const std::int64_t n = 64;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const pluss::Series x = coordinate(grid, 0), y = coordinate(grid, 1);

  // transport of a temperature-like field by a uniform wind u = (2, 1) m/s: u . grad f for f = sin x sin y
  const pluss::Series wind = x.mul(0.0).add(2.0).unsqueeze(0).unite(std::vector<pluss::Series>{x.mul(0.0).add(1.0).unsqueeze(0)}, 0);
  const pluss::Series f = x.sin().mul(y.sin());
  const pluss::Series exact = x.cos().mul(y.sin()).mul(2.0).add(x.sin().mul(y.cos()));
  out.writeln("advect(u, f) against 2 cos x sin y + sin x cos y: ", spatial::advect(wind, f, grid).sub(exact).abs().max().item<double>());

  // quantities carry their dimension through the operators: a pressure field gives a pressure per length
  const auto pressure = common::SeriesQuantity<common::Pressure>::from_si(f.mul(100.0));
  const auto gradient = spatial::partial(pressure, grid, 0);
  static_assert(std::same_as<decltype(gradient)::dimension, common::DimensionQuotient<common::Pressure, common::Length>>);
  out.writeln("d p / d x (a pressure per length), largest value: ", gradient.si_value().abs().max().item<double>(), "   (exact 100)");

  // the forward and backward differences compose into the compact Laplacian
  const pluss::Series composed = spatial::back_diff(spatial::fwd_diff(f, grid, 0), grid, 0);
  out.writeln("back_diff(fwd_diff(f)) against second_partial(f): ", composed.sub(spatial::second_partial(f, grid, 0)).abs().max().item<double>());
  // a batch of two fields in one call: leading axes are left alone
  const pluss::Series batch = f.unsqueeze(0).unite(std::vector<pluss::Series>{f.mul(3.0).unsqueeze(0)}, 0);
  const pluss::Series db = spatial::partial(batch, grid, 0);
  out.writeln("batch of two fields: derivative shape ", db.size(0), " x ", db.size(1), " x ", db.size(2), ", largest value of the tripled field / largest of the first = ", db.select(0, 1).abs().max().item<double>() / db.select(0, 0).abs().max().item<double>());
}
```

Output

```text
advect(u, f) against 2 cos x sin y + sin x cos y: 3.21e-03
d p / d x (a pressure per length), largest value: 9.96e+01   (exact 100)
back_diff(fwd_diff(f)) against second_partial(f): 1.24e-14
batch of two fields: derivative shape 2 x 64 x 64, largest value of the tripled field / largest of the first = 3.00e+00
```

`advect` agrees with the exact `2 cos x sin y + sin x cos y` to `3.2e-03`. The pressure gradient reaches `99.6` against the exact `100`, and its type is `Pressure / Length`, so the dimension travels with the value. `back_diff(fwd_diff(f))` equals `second_partial(f)` to `1.2e-14`, rounding, confirming the identity. The batch of two fields comes back with shape `2 x 64 x 64` and the tripled field has a derivative three times as large: batch axes pass straight through.

## Decisions

- Use `partial`, `second_partial` and `laplacian` for smooth fields; they are second-order accurate and cheap. Use `gradient`, `divergence` and `curl` when you want the vector forms.
- Use `wide_laplacian` when you need an operator that is exactly the divergence of the centred gradient. A pressure projection needs it: the divergence of the centred gradient is the wide stencil, not the compact one, as the second example shows.
- Use `fwd_diff` and `back_diff` together when you need a discrete adjoint pair (divergence as the negative transpose of the gradient).
- Pass `SeriesQuantity` arguments when the field has a unit, so the dimension of the result is checked; pass raw Series for the inner loop of a solver, after the units have been verified once.
- Keep the field in `Float64` unless you have measured that single precision is enough; the second difference of a smooth field loses digits as `h` shrinks.

## Pitfalls

Grid and field shapes must agree, and the grid is validated when you build it: at least three cells per axis, positive finite spacing, one spacing per axis. Operators check that the trailing axes of the field match the grid and that the axis you name exists.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;
using Len = common::ScalarQuantity<common::Length>;

int main() {
  OutputEngine out;
  const auto h = Len::from_si(0.1);
  try { spatial::make_periodic_grid({2, 8}, {h, h}); } catch (const pluss::Error &e) { out.writeln("two cells along an axis: ", e.message()); }
  try { spatial::make_periodic_grid({8, 8}, {h, Len::from_si(0.0)}); } catch (const pluss::Error &e) { out.writeln("zero spacing: ", e.message()); }
  try { spatial::make_periodic_grid({8, 8}, {h}); } catch (const pluss::Error &e) { out.writeln("one spacing for two axes: ", e.message()); }
  const auto grid = spatial::make_periodic_grid({8, 8}, {h, h});
  const pluss::Series f = pluss::series::zeros({8, 6}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  try { spatial::laplacian(f, grid); } catch (const pluss::Error &e) { out.writeln("field 8 x 6 on an 8 x 8 grid: ", e.message()); }
  try { spatial::partial(pluss::series::zeros({8, 8}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)), grid, 2); } catch (const pluss::Error &e) { out.writeln("axis 2 of a 2-D grid: ", e.message()); }
  try { spatial::divergence(pluss::series::zeros({8, 8}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)), grid); } catch (const pluss::Error &e) { out.writeln("divergence of a scalar field: ", e.message()); }
}
```

Output

```text
two cells along an axis:  natsci::spatial::make_periodic_grid: every extent must be at least three for a centered periodic stencil
zero spacing:  natsci::spatial::make_periodic_grid: spacing must be finite and positive
one spacing for two axes:  natsci::spatial::make_periodic_grid: shape and spacing rank mismatch
field 8 x 6 on an 8 x 8 grid:  natsci::spatial::laplacian: trailing spatial shape does not match the grid
axis 2 of a 2-D grid:  natsci::spatial::partial: spatial axis is out of range
divergence of a scalar field:  natsci::spatial::divergence: vector field has no component axis
```

Every message begins with the operator, so a failure deep in a solver still tells you where to look. The field of shape `8 x 6` against an `8 x 8` grid is the typical case: the field was made for a different grid.

A vector field puts its component axis **before** the grid axes. The program builds `(1, 2)` as a `[2, 4, 4]` array and takes its divergence, then repeats with the components stored last, and with three components on a two-dimensional grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;
using Len = common::ScalarQuantity<common::Length>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto h = Len::from_si(0.5);
  const auto grid = spatial::make_periodic_grid({4, 4}, {h, h});
  // one 2-D vector field stored as [component, x, y] (here (1, 2) everywhere): the component axis comes BEFORE the grid
  const pluss::Series ux = pluss::series::ones({4, 4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64));
  const pluss::Series u = ux.unsqueeze(0).unite(std::vector<pluss::Series>{ux.mul(2.0).unsqueeze(0)}, 0);
  out.writeln("divergence of a constant field: shape ", spatial::divergence(u, grid).dim(), "-D, largest value ", spatial::divergence(u, grid).abs().max().item<double>());
  try {  // components stored last, [x, y, component]
    spatial::divergence(u.permute({1, 2, 0}), grid);
  } catch (const pluss::Error &e) { out.writeln("components last: ", e.message()); }
  try {  // three components on a 2-D grid
    spatial::divergence(pluss::series::zeros({3, 4, 4}, pluss::Config{}.with_dtype(pluss::ScalarType::Float64)), grid);
  } catch (const pluss::Error &e) { out.writeln("three components, two axes: ", e.message()); }
}
```

Output

```text
divergence of a constant field: shape 2-D, largest value 0.0000
components last: natsci::spatial::divergence: component count must equal grid rank
three components, two axes: natsci::spatial::divergence: component count must equal grid rank
```

The constant field has zero divergence, as it should. Components stored last, `[x, y, component]`, and a component count that differs from the grid rank are both rejected with the same message, because in both cases the axis the operator reads as the component axis has the wrong size.

The centred first difference and the wide Laplacian skip a cell, so they cannot see a pattern that alternates sign from one cell to the next. The program applies each operator to a checkerboard on an 8-by-8 grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;
using Len = common::ScalarQuantity<common::Length>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto h = Len::from_si(0.5);
  const auto grid = spatial::make_periodic_grid({8, 8}, {h, h});
  // a checkerboard (-1)^(i+j) is the grid's highest-frequency pattern
  std::vector<double> v;
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) v.push_back(((i + j) % 2 == 0) ? 1.0 : -1.0);
  const pluss::Series checker = pluss::make_series(v, {8, 8}).to(pluss::ScalarType::Float64);
  out.writeln("largest |d/dx| of a checkerboard (centred difference): ", spatial::partial(checker, grid, 0).abs().max().item<double>());
  out.writeln("largest |d/dx| with the forward difference:            ", spatial::fwd_diff(checker, grid, 0).abs().max().item<double>());
  out.writeln("largest |laplacian| (compact):                         ", spatial::laplacian(checker, grid).abs().max().item<double>());
  out.writeln("largest |laplacian| (wide):                            ", spatial::wide_laplacian(checker, grid).abs().max().item<double>());
}
```

Output

```text
largest |d/dx| of a checkerboard (centred difference): 0.0000
largest |d/dx| with the forward difference:            4.0000
largest |laplacian| (compact):                         32.0000
largest |laplacian| (wide):                            0.0000
```

The centred derivative and the wide Laplacian both return `0`: they treat the checkerboard as constant. The forward difference sees it (`4.0000`) and so does the compact Laplacian (`32.0000`). A solver that uses only the centred operators therefore can neither detect nor remove a checkerboard.

## Where next

[Poisson solves](poisson_solves.md) inverts the Laplacian built here, directly in Fourier space or with multigrid. [Boxes and boundary rules](boxes_and_boundaries.md) replaces the wrap-around with value and flux conditions on a bounded box. The incompressible flow in [the Taylor-Green vortex](../../physics/phys_02_fluids/taylor_green_vortex.md) is made entirely of the operators on this page. The reference pages for `natsci::common::spatial::make_periodic_grid`, `PeriodicGrid`, `partial`, `second_partial`, `gradient`, `divergence`, `curl`, `laplacian`, `wide_laplacian`, `fwd_diff`, `back_diff` and `advect` list every overload.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="poisson_solves.html">
    <span class="label">Next</span>
    <span class="title">Poisson solves on periodic grids</span>
  </a>
</div>
<!-- apiref:nav:end -->
