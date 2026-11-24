# Boxes and boundary rules

[Back to Methods 7: Grids and operators](../meth_07_grids_and_operators.md)

Most real domains have walls: a bar held at fixed temperatures at its ends, a channel with no flow through its sides, a membrane with a fixed flux. On a periodic grid the edge wraps around; on a box you must say what happens at each wall. This page gives each wall a rule, differentiates fields with those rules, and solves Poisson's equation on the box with them.

## How it works

A box grid is made with `spatial::make_box_grid(shape, spacing)`, with one to three axes of at least two cells. Its cells are centred on `(i + 1/2) h`, and the walls lie halfway between the outermost cell centre and an imaginary cell beyond it. Each wall gets one rule, built with `value_bc(face, value)` (the field has this value on the wall) or `flux_bc(face, slope)` (the derivative of the field along the **outward** normal has this value). A `Face` names an axis and a `Side` (`Low` or `High`), and `make_bounds<D>(grid, rules)` collects exactly one rule for every face.

The rules work by supplying the cell that would lie just outside the box, the ghost cell. For a value rule the wall sits halfway between the first cell and its ghost, so the ghost is `2 g - u_0`; for a flux rule with outward slope `s` the ghost is `u_0 + s h`:

```{math}
u_{\mathrm{ghost}} = 2 g - u_0 \quad (\text{value } g), \qquad u_{\mathrm{ghost}} = u_0 + s\, h \quad (\text{outward slope } s).
```

`ghosts(u, grid, bounds, axis)` returns both ghost layers. With them, `partial`, `second_partial`, `gradient` and `laplacian` take the grid and the bounds and work on the same layout as the periodic operators. Bounded fields are `SeriesQuantity` values, so the rules carry dimensions: a value rule for a field of dimension `D` takes a `D`, and a flux rule takes a `D` per length. `solve_poisson(source, grid, bounds, options)` inverts the bounded Laplacian; the source is a plain Series and the bounds fix the solution's dimension.

## Ghost cells and the two kinds of rule

The program lays five cells of width `0.25` over a box and fills them with the straight line `u = 2 x` at the cell centres. It sets value rules `u = 0` at the low wall and `u = 2.5` at the high wall, which match the line, and prints the ghost cells and the derivative; then it sets flux rules `-2` at the low wall and `+2` at the high wall, the outward derivatives of the same line, and prints those ghosts and the second derivative.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;

using Field = common::Dimensionless;
using Flux = common::DimensionQuotient<Field, common::Length>;
using Value = common::ScalarQuantity<Field>;
using Slope = common::ScalarQuantity<Flux>;
using Len = common::ScalarQuantity<common::Length>;

// cell-centred coordinates of a box grid along one axis
static pluss::Series centres(const spatial::BoxGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(grid.shape().size(), 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // five cells of width 0.25 (a box of length 1.25); the field is the straight line u = 2 x measured at the cell centres
  const auto grid = spatial::make_box_grid({5}, {Len::from_si(0.25)});
  const pluss::Series x = centres(grid, 0);
  const auto u = common::SeriesQuantity<Field>::from_si(x.mul(2.0));
  out.writeln("cell values: ", u.si_value().host<double>()[0], " ", u.si_value().host<double>()[1], " ", u.si_value().host<double>()[2], " ", u.si_value().host<double>()[3], " ", u.si_value().host<double>()[4]);

  // value rules (u fixed on the wall) and flux rules (du/dx fixed on the wall)
  const auto walls = spatial::make_bounds<Field>(grid, {spatial::value_bc(spatial::Face{0, spatial::Side::Low}, Value::from_si(0.0)),
                                                        spatial::value_bc(spatial::Face{0, spatial::Side::High}, Value::from_si(2.5))});
  const auto g1 = spatial::ghosts(u, grid, walls, 0);
  out.writeln("value rules u = 0 and u = 2.5: ghost cells ", g1.low.si_value().item<double>(), " and ", g1.high.si_value().item<double>(), "  (2 * wall value - nearest cell)");
  out.writeln("   derivative at every cell: ", spatial::partial(u, grid, walls, 0).si_value().host<double>()[0], " ... ", spatial::partial(u, grid, walls, 0).si_value().host<double>()[4], "   (exact 2)");

  const auto slopes = spatial::make_bounds<Field>(grid, {spatial::flux_bc(spatial::Face{0, spatial::Side::Low}, Slope::from_si(-2.0)),
                                                         spatial::flux_bc(spatial::Face{0, spatial::Side::High}, Slope::from_si(2.0))});
  const auto g2 = spatial::ghosts(u, grid, slopes, 0);
  out.writeln("flux rules -2 (low wall) and +2 (high wall), the derivative along the outward normal: ghost cells ", g2.low.si_value().item<double>(), " and ", g2.high.si_value().item<double>(), "  (nearest cell + flux * cell width)");
  out.writeln("   second derivative with these rules, largest value: ", spatial::second_partial(u, grid, slopes, 0).si_value().abs().max().item<double>());
}
```

Output

```text
cell values: 0.250 0.750 1.250 1.750 2.250
value rules u = 0 and u = 2.5: ghost cells -0.250 and 2.750  (2 * wall value - nearest cell)
   derivative at every cell: 2.000 ... 2.000   (exact 2)
flux rules -2 (low wall) and +2 (high wall), the derivative along the outward normal: ghost cells -0.250 and 2.750  (nearest cell + flux * cell width)
   second derivative with these rules, largest value: 0.000
```

The line has cell values `0.25 ... 2.25`. The value rules give ghosts `-0.25` and `2.75`, the line continued past each wall, and the derivative is exactly `2` in every cell. The flux rules give the same ghosts by a different route (`0.25 - 2 * 0.25` and `2.25 + 2 * 0.25`), and the second derivative of a straight line is exactly zero.

## A bounded Poisson problem

To solve `u'' = f` on `(0, 1)` with `u = 0` at both walls, take `f = -pi^2 sin(pi x)`, whose solution is `sin(pi x)`. The program solves it on 16, 32 and 64 cells with the value rules and reports the largest error and the iterations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;

using Field = common::Dimensionless;
using Flux = common::DimensionQuotient<Field, common::Length>;
using Value = common::ScalarQuantity<Field>;
using Slope = common::ScalarQuantity<Flux>;
using Len = common::ScalarQuantity<common::Length>;

// cell-centred coordinates of a box grid along one axis
static pluss::Series centres(const spatial::BoxGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(grid.shape().size(), 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  // u'' = -pi^2 sin(pi x) on (0, 1) with u(0) = u(1) = 0 has the solution u = sin(pi x)
  const double pi = std::numbers::pi;
  double previous = 0.0;
  for (std::int64_t n : {16, 32, 64}) {
    const auto grid = spatial::make_box_grid({n}, {Len::from_si(1.0 / static_cast<double>(n))});
    const pluss::Series x = centres(grid, 0);
    const auto walls = spatial::make_bounds<Field>(grid, {spatial::value_bc(spatial::Face{0, spatial::Side::Low}, Value::from_si(0.0)),
                                                          spatial::value_bc(spatial::Face{0, spatial::Side::High}, Value::from_si(0.0))});
    const pluss::Series rhs = x.mul(pi).sin().mul(-pi * pi);
    const auto result = spatial::solve_poisson(rhs, grid, walls);
    const double error = result.solution.sub(x.mul(pi).sin()).abs().max().item<double>();
    out.writeln(n, " cells: error ", error, ", iterations ", result.iterations, previous > 0.0 ? "   error ratio " : "", previous > 0.0 ? std::to_string(previous / error) : "");
    previous = error;
  }
}
```

Output

```text
16 cells: error 3.20e-03, iterations 8
32 cells: error 8.03e-04, iterations 8   error ratio 3.991310
64 cells: error 2.01e-04, iterations 8   error ratio 3.997830
```

The error is `3.2e-03`, `8.0e-04` and `2.0e-04`: the ratio is almost exactly four each time, second order again, and the solver needs 8 iterations on every grid. A boundary-value problem therefore costs the same few iterations as the periodic ones, and the walls cost no accuracy.

## A sealed box: flux rules on every wall

When every wall is a flux rule the solution is determined only up to a constant, and the source must be compatible: with no flow through any wall, a net source has nowhere to go. The program solves `u'' = -pi^2 cos(pi x)` with zero flux at both walls (the source has zero total), checks `all_flux()`, and compares the solution with `cos(pi x)` after removing both means.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;

using Field = common::Dimensionless;
using Flux = common::DimensionQuotient<Field, common::Length>;
using Value = common::ScalarQuantity<Field>;
using Slope = common::ScalarQuantity<Flux>;
using Len = common::ScalarQuantity<common::Length>;

// cell-centred coordinates of a box grid along one axis
static pluss::Series centres(const spatial::BoxGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(grid.shape().size(), 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  // u'' = -pi^2 cos(pi x) on (0, 1) with u'(0) = u'(1) = 0: the solution cos(pi x) + C is known only up to the constant C
  const double pi = std::numbers::pi;
  const std::int64_t n = 64;
  const auto grid = spatial::make_box_grid({n}, {Len::from_si(1.0 / static_cast<double>(n))});
  const pluss::Series x = centres(grid, 0);
  const auto walls = spatial::make_bounds<Field>(grid, {spatial::flux_bc(spatial::Face{0, spatial::Side::Low}, Slope::from_si(0.0)),
                                                        spatial::flux_bc(spatial::Face{0, spatial::Side::High}, Slope::from_si(0.0))});
  out.writeln("every wall is a flux rule: all_flux() = ", walls.all_flux() ? "yes" : "no");
  const pluss::Series exact = x.mul(pi).cos();
  const pluss::Series rhs = exact.mul(-pi * pi);
  const auto result = spatial::solve_poisson(rhs, grid, walls);
  out.writeln("mean of the returned solution: ", std::abs(result.solution.mean().item<double>()), " (the constant is fixed to zero mean)");
  out.writeln("error against cos(pi x) - mean(cos(pi x)): ", result.solution.sub(exact.sub(exact.mean())).abs().max().item<double>());
  out.writeln("relative residual ", result.relative_residual.max().item<double>(), " after ", result.iterations, " iterations");
}
```

Output

```text
every wall is a flux rule: all_flux() = yes
mean of the returned solution: 1.14e-16 (the constant is fixed to zero mean)
error against cos(pi x) - mean(cos(pi x)): 2.01e-04
relative residual 4.32e-13 after 8 iterations
```

`all_flux()` is `yes`, so the solver removes the constant and returns a zero-mean solution (`1e-16`). The error against `cos(pi x)` with its mean removed is `2.0e-04`, the same second-order error as with value rules, and the solve converges in 8 iterations.

## Mixed walls in two dimensions

Walls can differ along each axis. The program solves `laplacian(u) = -2 pi^2 sin(pi x) cos(pi y)` on the unit square with `u = 0` on the two `x` walls and zero flux on the two `y` walls, whose solution is `sin(pi x) cos(pi y)`, on 16 to 128 cells per side. For each grid it also solves an irregular source with the default method and with plain conjugate gradients, to compare iteration counts.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;

using Field = common::Dimensionless;
using Flux = common::DimensionQuotient<Field, common::Length>;
using Value = common::ScalarQuantity<Field>;
using Slope = common::ScalarQuantity<Flux>;
using Len = common::ScalarQuantity<common::Length>;

// cell-centred coordinates of a box grid along one axis
static pluss::Series centres(const spatial::BoxGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(grid.shape().size(), 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

static pluss::Series noise(std::int64_t n) {
  std::vector<double> v(static_cast<std::size_t>(n * n));
  std::uint64_t state = 12345;
  for (double &e : v) {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    e = static_cast<double>(state >> 11) / 9007199254740992.0 - 0.5;
  }
  return pluss::make_series(v, {n, n}).to(pluss::ScalarType::Float64);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  // 2-D: u = sin(pi x) cos(pi y) on the unit square, u = 0 on the two x walls, du/dy = 0 on the two y walls; laplacian u = -2 pi^2 u
  const double pi = std::numbers::pi;
  for (std::int64_t n : {16, 24, 32, 64, 128}) {
    const Len h = Len::from_si(1.0 / static_cast<double>(n));
    const auto grid = spatial::make_box_grid({n, n}, {h, h});
    const auto rules = spatial::make_bounds<Field>(grid, {spatial::value_bc(spatial::Face{0, spatial::Side::Low}, Value::from_si(0.0)), spatial::value_bc(spatial::Face{0, spatial::Side::High}, Value::from_si(0.0)),
                                                          spatial::flux_bc(spatial::Face{1, spatial::Side::Low}, Slope::from_si(0.0)), spatial::flux_bc(spatial::Face{1, spatial::Side::High}, Slope::from_si(0.0))});
    const pluss::Series x = centres(grid, 0), y = centres(grid, 1);
    const pluss::Series exact = x.mul(pi).sin().mul(y.mul(pi).cos());
    const pluss::Series rhs = exact.mul(-2.0 * pi * pi);
    spatial::PoissonOptions cg;
    cg.method = spatial::PoissonMethod::ConjugateGradient;
    cg.solve.max_iterations = 5000;
    const auto smooth = spatial::solve_poisson(rhs, grid, rules);                 // Auto
    const auto a = spatial::solve_poisson(noise(n), grid, rules);                 // an irregular source needs many modes
    const auto b = spatial::solve_poisson(noise(n), grid, rules, cg);
    out.writeln(n, " x ", n, ": error ", smooth.solution.sub(exact).abs().max().item<double>(), " | irregular source: Auto ", a.iterations, " iterations, plain conjugate gradient ", b.iterations);
  }
}
```

Output

```text
16 x 16: error 3.19e-03 | irregular source: Auto 72 iterations, plain conjugate gradient 72
24 x 24: error 1.42e-03 | irregular source: Auto 112 iterations, plain conjugate gradient 112
32 x 32: error 8.02e-04 | irregular source: Auto 8 iterations, plain conjugate gradient 144
64 x 64: error 2.01e-04 | irregular source: Auto 8 iterations, plain conjugate gradient 288
128 x 128: error 5.02e-05 | irregular source: Auto 8 iterations, plain conjugate gradient 576
```

The error falls by four with every doubling, from `3.2e-03` to `5.0e-05`. For the irregular source, plain conjugate gradients need 72, 112, 144, 288 and 576 iterations as the grid grows, while the default method switches to multigrid from 32 cells per side and needs 8 from then on. On the smallest grids the default and plain conjugate gradients are the same method, which is why they match.

## Decisions

- Use `value_bc` when you know the field at the wall (a fixed temperature, a held potential) and `flux_bc` when you know its outward derivative (an insulated wall is zero flux; a heated wall is a positive flux for a quantity that leaves).
- Use a box grid whenever the edges do not wrap. The operators and the solver take the rules as an extra argument, and the rest of your code does not change.
- Leave the solver method at `Auto`: it uses conjugate gradients on the smallest grids and the multigrid-preconditioned version from 32 cells per side. Request `Multigrid` explicitly when you want it on a small grid.
- Keep source terms compatible with the walls: on a box with flux rules everywhere, the source must sum to zero.
- Give boundary values with the right dimension: the type system rejects a flux where a value is expected.

## Pitfalls

The sign convention of a flux rule is the one thing to get right: the slope is taken along the **outward** normal, so at a low wall it is minus the ordinary derivative. The program gives the line `u = 2 x` the flux `+2` at both walls, which is wrong at the low wall, and then the correct `-2` and `+2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;

using Field = common::Dimensionless;
using Flux = common::DimensionQuotient<Field, common::Length>;
using Value = common::ScalarQuantity<Field>;
using Slope = common::ScalarQuantity<Flux>;
using Len = common::ScalarQuantity<common::Length>;

// cell-centred coordinates of a box grid along one axis
static pluss::Series centres(const spatial::BoxGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(grid.shape().size(), 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // the line u = 2 x has du/dx = +2 at BOTH walls, but flux rules are given along the OUTWARD normal, which points the other way at the low wall
  const auto grid = spatial::make_box_grid({5}, {Len::from_si(0.25)});
  const auto u = common::SeriesQuantity<Field>::from_si(centres(grid, 0).mul(2.0));
  const auto wrong = spatial::make_bounds<Field>(grid, {spatial::flux_bc(spatial::Face{0, spatial::Side::Low}, Slope::from_si(2.0)), spatial::flux_bc(spatial::Face{0, spatial::Side::High}, Slope::from_si(2.0))});
  const auto right = spatial::make_bounds<Field>(grid, {spatial::flux_bc(spatial::Face{0, spatial::Side::Low}, Slope::from_si(-2.0)), spatial::flux_bc(spatial::Face{0, spatial::Side::High}, Slope::from_si(2.0))});
  out.writeln("low-wall flux +2: second derivative at the first cell ", spatial::second_partial(u, grid, wrong, 0).si_value().select(0, 0).item<double>(), "  (a straight line should give 0)");
  out.writeln("low-wall flux -2: second derivative at the first cell ", spatial::second_partial(u, grid, right, 0).si_value().select(0, 0).item<double>());
}
```

Output

```text
low-wall flux +2: second derivative at the first cell 16.000  (a straight line should give 0)
low-wall flux -2: second derivative at the first cell 0.000
```

With `+2` at the low wall the ghost cell is built on the wrong side and the second derivative in the first cell is `16`, where a straight line must give zero. With `-2` it is `0`. No error is raised, because both are legal rules; check a flux rule by applying it to a field whose derivative you know.

The solver validates what it can: a net source in a sealed box, a rule missing for a wall or on an axis the grid does not have, and the options that belong to periodic grids only.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace spatial = common::spatial;

using Field = common::Dimensionless;
using Flux = common::DimensionQuotient<Field, common::Length>;
using Value = common::ScalarQuantity<Field>;
using Slope = common::ScalarQuantity<Flux>;
using Len = common::ScalarQuantity<common::Length>;

// cell-centred coordinates of a box grid along one axis
static pluss::Series centres(const spatial::BoxGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(grid.shape().size(), 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}
int main() {
  OutputEngine out;
  const std::int64_t n = 16;
  const auto grid = spatial::make_box_grid({n}, {Len::from_si(1.0 / static_cast<double>(n))});
  const pluss::Series x = centres(grid, 0);
  const auto closed = spatial::make_bounds<Field>(grid, {spatial::flux_bc(spatial::Face{0, spatial::Side::Low}, Slope::from_si(0.0)), spatial::flux_bc(spatial::Face{0, spatial::Side::High}, Slope::from_si(0.0))});
  try {  // with no flow through either wall the source must add up to zero; this one adds up to 1
    spatial::solve_poisson(x.mul(0.0).add(1.0), grid, closed);
  } catch (const pluss::Error &e) { out.writeln("sealed box, source with a net total: ", e.message()); }
  try {  // only one of the two walls given
    spatial::make_bounds<Field>(grid, {spatial::value_bc(spatial::Face{0, spatial::Side::Low}, Value::from_si(0.0))});
  } catch (const pluss::Error &e) { out.writeln("one wall of two: ", e.message()); }
  try {  // a wall on an axis the grid does not have
    spatial::make_bounds<Field>(grid, {spatial::value_bc(spatial::Face{1, spatial::Side::Low}, Value::from_si(0.0)), spatial::value_bc(spatial::Face{0, spatial::Side::High}, Value::from_si(0.0))});
  } catch (const pluss::Error &e) { out.writeln("a wall on axis 1 of a 1-D grid: ", e.message()); }
  try {  // the Fourier method needs a periodic grid
    spatial::PoissonOptions options;
    options.method = spatial::PoissonMethod::Fft;
    spatial::solve_poisson(x.sin(), grid, spatial::make_bounds<Field>(grid, {spatial::value_bc(spatial::Face{0, spatial::Side::Low}, Value::from_si(0.0)), spatial::value_bc(spatial::Face{0, spatial::Side::High}, Value::from_si(0.0))}), options);
  } catch (const pluss::Error &e) { out.writeln("Fft on a box: ", e.message()); }
  try {  // the wide stencil is a periodic-grid operator
    spatial::PoissonOptions options;
    options.stencil = spatial::PoissonStencil::Wide;
    spatial::solve_poisson(x.sin(), grid, spatial::make_bounds<Field>(grid, {spatial::value_bc(spatial::Face{0, spatial::Side::Low}, Value::from_si(0.0)), spatial::value_bc(spatial::Face{0, spatial::Side::High}, Value::from_si(0.0))}), options);
  } catch (const pluss::Error &e) { out.writeln("wide stencil on a box: ", e.message()); }
}
```

Output

```text
sealed box, source with a net total:  natsci::spatial::solve_poisson: all-flux rhs violates the boundary compatibility condition in at least one batch lane
one wall of two:  natsci::spatial::make_bounds: expected exactly one rule per face
a wall on axis 1 of a 1-D grid:  natsci::spatial::make_bounds: face axis is out of range
Fft on a box:  natsci::spatial::solve_poisson: the Fft method is defined for periodic grids only
wide stencil on a box:  natsci::spatial::solve_poisson: the wide stencil is defined for periodic grids only
```

Each message names the cause. The sealed-box message is the physical one: with every wall a flux rule, a net source has nowhere to go, so the problem has no solution. The Fourier method and the wide stencil exist only for periodic grids.

## Where next

[Poisson solves](poisson_solves.md) covers the periodic case and the choice between the transform and the iterative methods. [Differential operators on periodic grids](periodic_operators.md) introduces the layouts that these bounded versions share. The reference pages for `natsci::common::spatial::BoxGrid`, `make_box_grid`, `Bounds`, `make_bounds`, `value_bc`, `flux_bc`, `ghosts` and the bounded `solve_poisson` list every argument.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="poisson_solves.html">
    <span class="label">Previous</span>
    <span class="title">Poisson solves on periodic grids</span>
  </a>
</div>
<!-- apiref:nav:end -->
