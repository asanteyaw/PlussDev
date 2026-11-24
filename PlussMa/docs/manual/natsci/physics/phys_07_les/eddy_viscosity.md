# Eddy-viscosity closures

[Back to Physics 7: Large-eddy simulation](../phys_07_les.md)

A turbulent flow has motion on every scale from the size of the domain down to scales far smaller than any grid you can afford. A large-eddy simulation resolves the large scales and models the effect of the small ones as an extra viscosity that drains energy from the resolved flow, as the small scales really do. This page computes that eddy viscosity from a resolved velocity field with the three closures the library provides, and shows how they differ.

## How it works

Filtering the momentum equation at a width {math}`\Delta` leaves a subgrid stress that the resolved field cannot compute. An eddy-viscosity closure replaces it by a viscous stress with a viscosity {math}`\nu_t` that depends on the resolved velocity gradient {math}`g_{ij} = \partial_j u_i`, whose symmetric part is the strain rate {math}`S_{ij}`. The total stress is then {math}`2(\nu + \nu_t) S_{ij}`, with the molecular viscosity {math}`\nu` and the eddy viscosity {math}`\nu_t`. The library provides three closures, all selected by a `les::Closure`:

* **Smagorinsky**, {math}`\nu_t = (C_s \Delta)^2 |S|` with {math}`|S| = \sqrt{2 S_{ij} S_{ij}}` and a default {math}`C_s = 0.17`. It is simple and robust and is nonzero wherever there is strain, even in a laminar flow.
* **WALE** (wall-adapting local eddy viscosity), {math}`\nu_t = (C_w \Delta)^2 \dfrac{(S^d_{ij} S^d_{ij})^{3/2}}{(S_{ij} S_{ij})^{5/2} + (S^d_{ij} S^d_{ij})^{5/4}}`, where {math}`S^d_{ij}` is the traceless symmetric part of the square of the gradient tensor and the default is {math}`C_w = 0.325`. It vanishes in a pure shear and goes to zero as {math}`y^3` at a wall without a damping function.
* **DynamicSmagorinsky**, which finds {math}`C_s^2` from the flow itself. A test filter of width {math}`\sqrt6\,\Delta` is applied to the resolved field, and the Germano identity relates the stresses at the two filter widths: {math}`C_s^2 = \langle L_{ij} M_{ij} \rangle / \langle M_{ij} M_{ij} \rangle` with {math}`L_{ij}` the Leonard stress and {math}`M_{ij}` the model difference tensor. It needs directions in which the average can be taken, so it works on periodic grids only.

`les::eddy_viscosity(state, grid, closure)` returns {math}`\nu_t` in every cell as a quantity of kinematic viscosity, for an `IncompressibleState` on a periodic grid or, for the first two closures, on a box grid with `FlowBounds`. The fields of `les::Closure` are `model`, `constant` (a negative value selects the default), `width` (zero selects the geometric mean of the cell sizes), and, for the dynamic model, `clip` and `local_average`.

## The Smagorinsky viscosity of a vortex

The Taylor-Green vortex {math}`u = (-\cos x \sin y,\ \sin x \cos y)` has the strain components {math}`S_{11} = -S_{22} = \sin x \sin y` and {math}`S_{12} = 0`, so {math}`|S| = 2 |\sin x \sin y|` and the Smagorinsky viscosity is {math}`(C_s\Delta)^2 \cdot 2|\sin x \sin y|` with {math}`\Delta` the cell size. The program evaluates the closure on a `16 x 16` grid and compares it with this formula.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace fluids = natsci::physics::fluids;
namespace les = fluids::les;

static constexpr double two_pi = 2.0 * std::numbers::pi;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const std::int64_t n = 16;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  // the Taylor-Green vortex u = (-cos x sin y, sin x cos y): its strain is S11 = -S22 = sin x sin y and S12 = 0, so |S| = 2 |sin x sin y|
  const auto velocity = x.cos().neg().mul(y.sin()).unsqueeze(0).unite(std::vector<pluss::Series>{x.sin().mul(y.cos()).unsqueeze(0)}, 0);
  const auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(velocity),
                                        common::SeriesQuantity<common::Pressure>::from_si(velocity.select(0, 0).mul(0.0)), grid);

  les::Closure smagorinsky;
  smagorinsky.model = les::Model::Smagorinsky;  // the constant defaults to 0.17
  const auto nut = les::eddy_viscosity(state, grid, smagorinsky).si_value();
  const double delta = two_pi / n;  // the filter width defaults to the cell size
  const auto exact = x.sin().mul(y.sin()).abs().mul(2.0 * std::pow(0.17 * delta, 2));
  out.writeln("filter width ", delta, ", Smagorinsky constant 0.17");
  out.writeln("largest eddy viscosity ", nut.max().item<double>(), " m^2/s; from the formula (Cs Delta)^2 |S| = ", exact.max().item<double>());
  out.writeln("largest difference between the two fields ", nut.sub(exact).abs().max().item<double>());
  out.writeln("eddy viscosity where the strain is largest (cell 4, 4): ", nut.select(0, 4).select(0, 4).item<double>(), "; next to the point where it vanishes (cell 0, 0): ", nut.select(0, 0).select(0, 0).item<double>());
}
```

Output

```text
filter width 0.392699, Smagorinsky constant 0.17
largest eddy viscosity 0.008356 m^2/s; from the formula (Cs Delta)^2 |S| = 0.008574
largest difference between the two fields 0.000219
eddy viscosity where the strain is largest (cell 4, 4): 0.008356; next to the point where it vanishes (cell 0, 0): 0.000331
```

The largest eddy viscosity from the library is `0.008356` against `0.008574` from the formula, and the fields differ by at most `0.000219`: the library takes the strain from central differences on the grid, which under-read the gradient of a wave a few cells wide by a factor {math}`\sin(kh)/(kh)`. The viscosity follows the strain: `0.008356` in the cell where the strain is largest and only `0.000331` in a cell next to the point where it vanishes.

## Three closures on two flows

Here is what separates the closures. The program applies each to two flows on a `32 x 32` grid: a pure shear {math}`u = \sin y`, and the Taylor-Green vortex, and prints the largest eddy viscosity in each case.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace fluids = natsci::physics::fluids;
namespace les = fluids::les;

static constexpr double two_pi = 2.0 * std::numbers::pi;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

static fluids::IncompressibleState make(const spatial::PeriodicGrid &grid, const pluss::Series &u, const pluss::Series &v) {
  const auto velocity = u.unsqueeze(0).unite(std::vector<pluss::Series>{v.unsqueeze(0)}, 0);
  return fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(velocity), common::SeriesQuantity<common::Pressure>::from_si(u.mul(0.0)), grid);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(3).with_sep(""));
  const std::int64_t n = 32;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const auto zero = x.mul(0.0);

  // two flows: a pure shear u = sin y, which is all velocity gradient in one direction, and the Taylor-Green vortex, a smooth flow of strain and rotation
  const auto shear = make(grid, y.sin(), zero);
  const auto vortex = make(grid, x.cos().neg().mul(y.sin()), x.sin().mul(y.cos()));
  const struct { const char *name; les::Model model; } models[] = {{"Smagorinsky", les::Model::Smagorinsky}, {"WALE", les::Model::Wale}, {"dynamic Smagorinsky", les::Model::DynamicSmagorinsky}};
  out.writeln("largest eddy viscosity of each model");
  out.writeln("                      pure shear   Taylor-Green vortex");
  for (const auto &m : models) {
    les::Closure closure;
    closure.model = m.model;
    out.writeln(m.name, ":  ", les::eddy_viscosity(shear, grid, closure).si_value().max().item<double>(), "   ", les::eddy_viscosity(vortex, grid, closure).si_value().max().item<double>());
  }
}
```

Output

```text
largest eddy viscosity of each model
                      pure shear   Taylor-Green vortex
Smagorinsky:  1.102e-03   2.193e-03
WALE:  0.000e+00   2.751e-03
dynamic Smagorinsky:  0.000e+00   1.047e-19
```

On the pure shear the Smagorinsky model gives `1.102e-03`, which is {math}`(0.17 \cdot 0.196)^2 \cdot 1`: it sees strain and adds viscosity, although the flow is laminar. The WALE model gives exactly zero, because a pure shear has no rotation-strain interaction that its numerator picks up. On the vortex WALE gives `2.751e-03` and Smagorinsky `2.193e-03`; the dynamic model gives `1.047e-19`, round-off. A flow of a single smooth mode has no small-scale content: the Germano identity finds nothing to model, and the dynamic model switches itself off, which is the behaviour you want from a closure in a flow that is already resolved.

## The constant and the filter width

The eddy viscosity is a product of the square of the constant and the square of the width. The program doubles each in turn and prints the maximum viscosity of the vortex, then switches to the WALE model with its own default constant.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace fluids = natsci::physics::fluids;
namespace les = fluids::les;

static constexpr double two_pi = 2.0 * std::numbers::pi;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const std::int64_t n = 16;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto x = coordinate(grid, 0), y = coordinate(grid, 1);
  const auto velocity = x.cos().neg().mul(y.sin()).unsqueeze(0).unite(std::vector<pluss::Series>{x.sin().mul(y.cos()).unsqueeze(0)}, 0);
  const auto state = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(velocity), common::SeriesQuantity<common::Pressure>::from_si(velocity.select(0, 0).mul(0.0)), grid);

  // the eddy viscosity is (C Delta)^2 times a property of the flow, so it scales with the square of the constant and of the filter width
  les::Closure closure;
  closure.model = les::Model::Smagorinsky;
  const double base = les::eddy_viscosity(state, grid, closure).si_value().max().item<double>();
  out.writeln("default constant 0.17 and width = cell size: ", base);
  closure.constant = 0.34;
  out.writeln("constant doubled:                            ", les::eddy_viscosity(state, grid, closure).si_value().max().item<double>(), "  (ratio ", les::eddy_viscosity(state, grid, closure).si_value().max().item<double>() / base, ")");
  closure.constant = -1.0;
  closure.width = 2.0 * two_pi / n;
  out.writeln("filter width doubled:                        ", les::eddy_viscosity(state, grid, closure).si_value().max().item<double>(), "  (ratio ", les::eddy_viscosity(state, grid, closure).si_value().max().item<double>() / base, ")");
  // the other two models have their own default constants
  closure.width = 0.0;
  closure.model = les::Model::Wale;
  out.writeln("WALE with its default constant 0.325:        ", les::eddy_viscosity(state, grid, closure).si_value().max().item<double>());
}
```

Output

```text
default constant 0.17 and width = cell size: 0.008356
constant doubled:                            0.033422  (ratio 4.000000)
filter width doubled:                        0.033422  (ratio 4.000000)
WALE with its default constant 0.325:        0.010475
```

Doubling the constant multiplies the largest viscosity by exactly `4.000000`, and so does doubling the filter width. The two are interchangeable in the Smagorinsky model, which is why a constant is only meaningful together with a stated filter width. The default width is the cell size; setting `closure.width` to a larger multiple of the cell size means the model represents a larger part of the flow. The WALE viscosity with its default `0.325` is `0.010475` here, about 1.25 times the Smagorinsky value, because its default constant is larger.

## Decisions

- Use **WALE** as the first choice for wall-bounded flow and for transitional flow: it has the right wall behaviour without a damping function, and it adds no viscosity to a laminar shear.
- Use **Smagorinsky** for homogeneous turbulence or when you need the simplest, most predictable closure. Its constant is tuned: `0.17` is the isotropic-turbulence value, and values from about `0.1` to `0.2` are used in practice.
- Use **DynamicSmagorinsky** when no good constant is known, and you have a periodic (homogeneous) flow. Keep `clip = true` (the default) unless you want to allow backscatter, which makes a run unstable if the model is the only damping.
- Leave `width` at its default of the geometric-mean cell size unless the grid is strongly stretched: in that case a hand-chosen width is a deliberate modelling choice, and you should say what it is.

## Pitfalls

Smagorinsky adds viscosity to a laminar flow. The flow {math}`u = \sin y` decays at the pure viscous rate, so any extra loss is the closure's. The program runs it for `t = 0.5` with a molecular viscosity of `0.001` and the Smagorinsky and WALE closures.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace fluids = natsci::physics::fluids;
namespace les = fluids::les;

static constexpr double two_pi = 2.0 * std::numbers::pi;

static pluss::Series coordinate(const spatial::PeriodicGrid &grid, std::int64_t axis) {
  const std::int64_t n = grid.shape()[static_cast<std::size_t>(axis)];
  std::vector<std::int64_t> view(2, 1);
  view[static_cast<std::size_t>(axis)] = n;
  return pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).mul(grid.spacing(axis).si_value()).reshape(view).expand(grid.shape(), true);
}

static double kinetic_energy(const fluids::IncompressibleState &state) { return 0.5 * state.velocity().si_value().square().sum(0, false).mean().item<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const std::int64_t n = 32;
  const auto h = common::ScalarQuantity<common::Length>::from_si(two_pi / static_cast<double>(n));
  const auto grid = spatial::make_periodic_grid({n, n}, {h, h});
  const auto y = coordinate(grid, 1);
  // a laminar shear flow u = sin y has nothing to model: any viscosity the closure adds is an error of the model, not physics
  const auto u = y.sin();
  const auto velocity = u.unsqueeze(0).unite(std::vector<pluss::Series>{u.mul(0.0).unsqueeze(0)}, 0);
  const auto start = fluids::make_state(common::SeriesQuantity<common::Speed>::from_si(velocity), common::SeriesQuantity<common::Pressure>::from_si(u.mul(0.0)), grid);
  const auto density = common::ScalarQuantity<fluids::Density>::from_si(1.0);
  const auto viscosity = common::ScalarQuantity<fluids::KinematicViscosity>::from_si(1e-3);
  fluids::StepOptions options;
  options.rule = fluids::StepRule::Rk3;
  options.guard.enforce = false;
  const double e0 = kinetic_energy(start);
  out.writeln("exact decay of the laminar flow: ", e0 * std::exp(-2.0 * 1e-3 * 0.5), " after t = 0.5, from ", e0);
  for (auto model : {les::Model::Smagorinsky, les::Model::Wale}) {
    les::Closure closure;
    closure.model = model;
    auto state = start;
    for (int k = 0; k < 50; ++k)
      state = les::advance(state, grid, density, viscosity, closure, common::ScalarQuantity<common::Time>::from_si(k * 0.01), common::ScalarQuantity<common::Time>::from_si(0.01), options).state;
    out.writeln(model == les::Model::Smagorinsky ? "Smagorinsky" : "WALE       ", ": kinetic energy after t = 0.5 is ", kinetic_energy(state));
  }
}
```

Output

```text
exact decay of the laminar flow: 0.249750 after t = 0.5, from 0.250000
Smagorinsky: kinetic energy after t = 0.5 is 0.249522
WALE       : kinetic energy after t = 0.5 is 0.249753
```

The exact molecular decay brings the kinetic energy from `0.25` to `0.249750`. With the WALE closure the energy after `t = 0.5` is `0.249753`, the exact value to within the discretisation error; with Smagorinsky it is `0.249522`: the model drained almost twice as much energy as the molecular viscosity does, from a flow in which there is no turbulence to model. For a flow that may be laminar over part of the domain, choose WALE or the dynamic model.

## Where next

The next page, [Decaying flow with a closure](decaying_flow.md), advances a flow with a closure in the momentum equation and checks the energy budget. For the unresolved scales of a wall-bounded flow modelled in the mean, read [Physics 8](../phys_08_rans.md). The resolved part of this chapter is the incompressible solver of [the Taylor-Green vortex](../phys_02_fluids/taylor_green_vortex.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="decaying_flow.html">
    <span class="label">Next</span>
    <span class="title">Decaying flow with a closure</span>
  </a>
</div>
<!-- apiref:nav:end -->
