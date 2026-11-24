# Poisson solves on periodic grids

[Back to Methods 14: Grids and operators](../meth_14_grids_and_operators.md)

Poisson's equation, `laplacian(u) = f`, turns up whenever a field is set by its sources: the pressure that keeps a flow incompressible, the electrostatic potential of a charge density, the steady temperature under a heat source. This page solves it on a periodic grid. You give the source `f` and the grid, and `spatial::solve_poisson` returns the potential `u` together with diagnostics, choosing between an exact Fourier solve and iterative solvers.

## How it works

On a periodic domain the discrete Laplacian is diagonal in the Fourier basis, so the equation is solved by one forward transform, one division and one inverse transform. For a grid with `n` cells of width `h` the eigenvalue of the compact three-point Laplacian along one axis at integer frequency `j` is

```{math}
\lambda_j = -\frac{4}{h^2} \sin^2\!\Big(\frac{\pi j}{n}\Big),
```

and the eigenvalues of several axes add. The constant mode has eigenvalue zero, which is why two things follow: the source must have zero mean (nothing can balance a net source on a closed domain), and the solution is only defined up to a constant, which the solver sets so that the mean is zero. The wide stencil, the divergence of the centred gradient, has eigenvalue `-sin^2(2 pi j / n) / h^2` and also vanishes on the alternating mode of an even axis; both null components are removed.

`PoissonOptions::method` chooses how: `Fft` is the exact transform solve (CPU, single or double precision); `ConjugateGradient` is preconditioned conjugate gradients; `Multigrid` is conjugate gradients preconditioned by a multigrid V-cycle, whose iteration count does not grow with the grid; `Auto`, the default, uses the transform when it applies. `PoissonOptions::stencil` chooses `Compact` or `Wide`. The result holds `solution`, per-lane `residual_norm` and `relative_residual`, `converged`, `breakdown`, and `iterations` (zero for the transform). Leading axes of the source are independent lanes solved in one call.

## A solve and its error

The simplest test is a source whose potential you know. With `u = sin x sin y` on a grid over one period, `laplacian(u) = -2 u`, so the source is `-2 sin x sin y`. The program solves it on 16, 32 and 64 cells per side with the default options and prints the largest error against `u`, the relative residual of the discrete equation, and the iteration count.

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
  for (std::int64_t n : {16, 32, 64}) {
    const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
    const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
    const pluss::Series x = coordinate(grid, 0), y = coordinate(grid, 1);
    const pluss::Series exact = x.sin().mul(y.sin());           // we want u with laplacian(u) = rhs ...
    const pluss::Series rhs = exact.mul(-2.0);                   // ... and rhs = -2 sin x sin y, so u = sin x sin y
    const auto result = spatial::solve_poisson(rhs, grid);
    out.writeln(n, " x ", n, ": error ", result.solution.sub(exact).abs().max().item<double>(), ", relative residual ", result.relative_residual.max().item<double>(),
                ", iterations ", result.iterations);
  }
}
```

Output

```text
16 x 16: error 1.25e-02, relative residual 1.76e-15, iterations 0
32 x 32: error 3.19e-03, relative residual 5.55e-15, iterations 0
64 x 64: error 8.02e-04, relative residual 1.99e-14, iterations 0
```

The residual is at rounding level, `1e-15` to `2e-14`: the transform solves the discrete equation exactly. The error against the true potential is `1.25e-02`, `3.19e-03` and `8.0e-04`, falling by four each time the grid is refined, because the discrete operator itself approximates the Laplacian to second order. Zero iterations confirms that the default took the transform route.

## Choosing the method

The three methods solve the same discrete equation, so they must agree; they differ in cost. The program solves an irregular, zero-mean source (a fixed pseudo-random sequence, so every mode of the grid is excited) on grids of 32, 64 and 128 cells per side with each method, and prints the iterations and the largest difference from the transform solution.

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
// a reproducible irregular right-hand side: a fixed linear congruential sequence with its mean removed
static pluss::Series noise(std::int64_t n) {
  std::vector<double> v(static_cast<std::size_t>(n * n));
  std::uint64_t state = 12345;
  for (double &e : v) {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    e = static_cast<double>(state >> 11) / 9007199254740992.0 - 0.5;
  }
  const pluss::Series s = pluss::make_series(v, {n, n}).to(pluss::ScalarType::Float64);
  return s.sub(s.mean());
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const double two_pi = 2.0 * std::numbers::pi;
  for (std::int64_t n : {32, 64, 128}) {
    const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
    const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
    const pluss::Series rhs = noise(n);
    spatial::PoissonOptions spectral, cg, multigrid;
    spectral.method = spatial::PoissonMethod::Fft;
    cg.method = spatial::PoissonMethod::ConjugateGradient;
    multigrid.method = spatial::PoissonMethod::Multigrid;
    cg.solve.relative_tolerance = multigrid.solve.relative_tolerance = 1e-10;
    cg.solve.max_iterations = 5000;
    const auto a = spatial::solve_poisson(rhs, grid, spectral);
    const auto b = spatial::solve_poisson(rhs, grid, cg);
    const auto c = spatial::solve_poisson(rhs, grid, multigrid);
    out.writeln(n, " x ", n, ": Fft ", a.iterations, " iterations | conjugate gradient ", b.iterations, " iterations, differs from Fft by ", b.solution.sub(a.solution).abs().max().item<double>(),
                " | multigrid ", c.iterations, " iterations, differs by ", c.solution.sub(a.solution).abs().max().item<double>());
  }
}
```

Output

```text
32 x 32: Fft 0 iterations | conjugate gradient 72 iterations, differs from Fft by 1.15e-12 | multigrid 8 iterations, differs by 6.83e-13
64 x 64: Fft 0 iterations | conjugate gradient 152 iterations, differs from Fft by 7.09e-13 | multigrid 8 iterations, differs by 5.96e-13
128 x 128: Fft 0 iterations | conjugate gradient 288 iterations, differs from Fft by 6.75e-13 | multigrid 8 iterations, differs by 3.34e-13
```

Plain conjugate gradients need 72, 152 and 288 iterations: the count roughly doubles whenever the grid does. The multigrid-preconditioned version needs 8 on every grid, which is what makes it the choice for bounded domains where no transform applies. Both agree with the transform to about `1e-12`.

## The wide stencil and several sources at once

A projection method pairs the centred gradient and divergence, so the operator it inverts is the wide Laplacian. Select it with `stencil = Wide`. The program also stacks three sources, the same pattern times 1, 2 and 3, along a leading axis, and solves them in one call with both stencils.

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
  const pluss::Series exact = x.sin().mul(y.sin());
  // three sources in ONE call: the same pattern scaled by 1, 2 and 3 in a leading batch axis
  const pluss::Series scale = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3, 1, 1}).to(pluss::ScalarType::Float64);
  const pluss::Series rhs = exact.mul(-2.0).unsqueeze(0).mul(scale);
  spatial::PoissonOptions compact, wide;
  wide.stencil = spatial::PoissonStencil::Wide;
  const auto a = spatial::solve_poisson(rhs, grid, compact);
  const auto b = spatial::solve_poisson(rhs, grid, wide);
  out.writeln("batched solution shape: ", a.solution.size(0), " x ", a.solution.size(1), " x ", a.solution.size(2));
  for (std::int64_t k = 0; k < 3; ++k)
    out.writeln("lane ", k, ": compact error ", a.solution.select(0, k).sub(exact.mul(static_cast<double>(k + 1))).abs().max().item<double>(), ", wide error ",
                b.solution.select(0, k).sub(exact.mul(static_cast<double>(k + 1))).abs().max().item<double>());
  out.writeln("wide solution satisfies the wide equation: residual ", spatial::wide_laplacian(b.solution, grid).sub(rhs).abs().max().item<double>());
  out.writeln("compact solution satisfies the compact equation: residual ", spatial::laplacian(a.solution, grid).sub(rhs).abs().max().item<double>());
}
```

Output

```text
batched solution shape: 3 x 32 x 32
lane 0: compact error 3.19e-03, wide error 1.28e-02
lane 1: compact error 6.38e-03, wide error 2.57e-02
lane 2: compact error 9.56e-03, wide error 3.85e-02
wide solution satisfies the wide equation: residual 3.91e-14
compact solution satisfies the compact equation: residual 7.46e-14
```

The solution has shape `3 x 32 x 32`, one potential per lane, and the errors scale with the source: `3.2e-03`, `6.4e-03`, `9.6e-03` for the compact stencil. The wide stencil is four times less accurate on the same grid (`1.3e-02` for the first lane) because its stencil spans `2h`. Each solution satisfies the equation of its own stencil to `1e-14`, so choose the stencil by the operator you are inverting, not by the error.

## Decisions

- Leave `method` at `Auto` for periodic grids unless you need the iterative path for a reason such as a GPU field; the transform is exact and costs `O(N log N)`.
- Use `Multigrid` for bounded domains and large grids when you request an iterative method: its iteration count does not grow with the grid. Use `ConjugateGradient` only as a baseline.
- Use `Wide` when the solution feeds a centred divergence, as in a pressure projection; use `Compact` for everything else, where it is the more accurate of the two.
- Keep `require_convergence` at its default of `true` so that a failed iterative solve is an error; set it to `false` only when you want to inspect `converged` and the residual yourself.
- Subtract the mean of a source before solving when you know it is zero up to rounding: the check tolerance `mean_tolerance` is `1e-10`.

## Pitfalls

Three inputs are rejected: a source with a non-zero mean, the wide stencil with multigrid (the wide operator decouples into independent sub-grids, so use `Fft` or `ConjugateGradient`), and a source of integer type.

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
  const double two_pi = 2.0 * std::numbers::pi;
  const std::int64_t n = 16;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const pluss::Series x = coordinate(grid, 0), y = coordinate(grid, 1);
  try {  // a source whose total is not zero: on a periodic domain nothing can balance it
    spatial::solve_poisson(x.sin().mul(y.sin()).add(1.0), grid);
  } catch (const pluss::Error &e) { out.writeln("non-zero mean: ", e.message()); }
  try {  // the wide stencil with multigrid
    spatial::PoissonOptions options;
    options.stencil = spatial::PoissonStencil::Wide;
    options.method = spatial::PoissonMethod::Multigrid;
    spatial::solve_poisson(x.sin().mul(y.sin()), grid, options);
  } catch (const pluss::Error &e) { out.writeln("wide + multigrid: ", e.message()); }
  try {  // an integer right-hand side
    spatial::solve_poisson(pluss::series::zeros({n, n}, pluss::Config{}.with_dtype(pluss::ScalarType::Int64)), grid);
  } catch (const pluss::Error &e) { out.writeln("integer source: ", e.message()); }
}
```

Output

```text
non-zero mean:  natsci::spatial::solve_poisson: periodic rhs must have zero spatial mean in every batch lane
wide + multigrid:  natsci::spatial::solve_poisson: the wide stencil is solved by Fft or ConjugateGradient (its parity classes decouple)
integer source:  natsci::spatial::solve_poisson: field must have a floating or complex dtype
```

The first message is the physical one: on a periodic domain nothing balances a net source, so there is no solution. Fix it by subtracting the mean, after checking that the net source really is a modelling artifact.

The solution is fixed only up to a constant, and the solver returns the one with zero mean. Two potentials that differ by 5 have the same Laplacian, so solving for the Laplacian of `u + 5` returns `u`.

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
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const double two_pi = 2.0 * std::numbers::pi;
  const std::int64_t n = 32;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const pluss::Series x = coordinate(grid, 0), y = coordinate(grid, 1);
  // two potentials that differ by the constant 5 have the same laplacian, so the equation cannot tell them apart
  const pluss::Series u = x.sin().mul(y.sin());
  const auto result = spatial::solve_poisson(spatial::laplacian(u.add(5.0), grid), grid);
  out.writeln("mean of the potential that was solved for: ", u.add(5.0).mean().item<double>());
  out.writeln("mean of the returned solution:             ", std::abs(result.solution.mean().item<double>()));
  out.writeln("returned solution + 5 against the original, largest difference: ", result.solution.add(5.0).sub(u.add(5.0)).abs().max().item<double>());
}
```

Output

```text
mean of the potential that was solved for: 5.000000
mean of the returned solution:             0.000000
returned solution + 5 against the original, largest difference: 0.000000
```

The mean of the potential that was solved for is `5.000000`, the mean of the returned solution is `0.000000`, and adding 5 back reproduces the original to six digits. If a physical boundary value fixes the constant, add it yourself.

An iterative method stops at `max_iterations`. By default that is an error. With `require_convergence = false` you get the unconverged iterate and the diagnostics instead.

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
// a reproducible irregular right-hand side: a fixed linear congruential sequence with its mean removed
static pluss::Series noise(std::int64_t n) {
  std::vector<double> v(static_cast<std::size_t>(n * n));
  std::uint64_t state = 12345;
  for (double &e : v) {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    e = static_cast<double>(state >> 11) / 9007199254740992.0 - 0.5;
  }
  const pluss::Series s = pluss::make_series(v, {n, n}).to(pluss::ScalarType::Float64);
  return s.sub(s.mean());
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2).with_sep(""));
  const double two_pi = 2.0 * std::numbers::pi;
  const std::int64_t n = 64;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const pluss::Series rhs = noise(n);

  spatial::PoissonOptions few;
  few.method = spatial::PoissonMethod::ConjugateGradient;
  few.solve.max_iterations = 2;
  try { spatial::solve_poisson(rhs, grid, few); } catch (const pluss::Error &e) { out.writeln("two iterations allowed: ", e.message()); }
  few.require_convergence = false;
  const auto r = spatial::solve_poisson(rhs, grid, few);
  out.writeln("with require_convergence = false: converged = ", r.converged.all().item<bool>() ? "yes" : "no", ", relative residual ", r.relative_residual.max().item<double>(), " after ", r.iterations, " iterations");
}
```

Output

```text
two iterations allowed: natsci::spatial::solve_poisson: conjugate gradient did not converge
with require_convergence = false: converged = no, relative residual 3.65e-01 after 2 iterations
```

Two iterations of conjugate gradients on an irregular source stop with the error `conjugate gradient did not converge`. With the check off, the same call returns with `converged = no` and a relative residual of `0.365`, which is the information you need to decide how many more iterations to allow.

## Where next

[Boxes and boundary rules](boxes_and_boundaries.md) solves the same equation on a bounded box with value and flux conditions on each wall. The incompressible solver in [the Taylor-Green vortex](../../physics/phys_02_fluids/taylor_green_vortex.md) solves a Poisson equation in its projection step to keep the velocity free of divergence. The reference pages for `natsci::common::spatial::solve_poisson`, `PoissonOptions`, `PoissonMethod`, `PoissonStencil` and `PoissonResult` list every field.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
