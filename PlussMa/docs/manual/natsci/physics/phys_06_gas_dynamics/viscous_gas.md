# A viscous gas

[Back to Physics 6: Gas dynamics](../phys_06_gas_dynamics.md)

Shocks are nearly inviscid, but boundary layers, mixing layers and the decay of any smooth wave are not. This page adds viscosity and heat conduction to the compressible solver: how they limit the step, how a shear wave decays at the exact rate, and how the viscosity of air depends on temperature.

## How it works

With transport the momentum equation gains the divergence of a Newtonian stress and the energy equation the work of that stress and a Fourier heat flux:

```{math}
\begin{aligned}
&\tau_{ij} = \mu\left(\partial_j u_i + \partial_i u_j - \tfrac23 \delta_{ij}\, \partial_k u_k\right) + \zeta\, \delta_{ij}\, \partial_k u_k, \\
&q = -k\,\nabla T.
\end{aligned}
```

You give the coefficients in `gas::Transport` (`shear` for {math}`\mu`, `bulk` for {math}`\zeta`, `heat` for {math}`k`) and put it in `options.step.transport`. For air over a range of temperatures set `options.step.sutherland` to a `gas::SutherlandLaw` instead; it gives the viscosity as a function of the local temperature,

```{math}
\mu(T) = \mu_0 \left(\frac{T}{T_0}\right)^{3/2} \frac{T_0 + S}{T + S},
```

and the conductivity from the Prandtl number, {math}`k = \mu c_p / \mathrm{Pr}`. The Sutherland defaults are those of air (`1.716e-5` Pa s at 273.15 K, `S = 110.4` K, Pr `0.71`). `gas::transport(state, grid, model, law)` returns the fields, `gas::stress` the stress tensor and `gas::heat_flux` the heat flux of a state. Viscosity adds a second stability limit: explicit diffusion needs {math}`\Delta t \le \tfrac12 \Delta x^2/\nu_{\max}` for the largest of the momentum and thermal diffusivities, and `gas::limits` reports both limits and the smaller one.

## The two stability limits

A shear wave in a periodic unit square, `v = 0.01 sin(2 pi x)` in a gas of density 1 and pressure 1, on a `32 x 4` grid. The program prints the acoustic limit, the diffusive limit and the step that is allowed, for four values of the shear viscosity.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace gas = natsci::physics::fluids::gas;

static common::ScalarQuantity<common::Length> metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
static common::ScalarQuantity<common::Time> seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  gas::Model model;  // dimensionless gas, T = p / rho
  model.gas_constant = common::ScalarQuantity<gas::SpecificGasConstant>::from_si(1.0);

  // a shear wave in a periodic square of side 1: the cross-stream speed v = 0.01 sin(2 pi x) in a uniform gas of density 1 and pressure 1
  const std::int64_t n = 32, m = 4;
  const auto grid = spatial::make_periodic_grid({n, m}, {metres(1.0 / n), metres(1.0 / m)});
  const auto x = pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).div(static_cast<double>(n)).reshape({n, 1}).expand({n, m}, true);
  const auto rho = pluss::series::ones({n, m}, pluss::ScalarType::Float64);
  const auto v = x.mul(2.0 * std::numbers::pi).sin().mul(0.01);
  const auto momentum = pluss::series::zeros_like(v).unsqueeze(0).unite(std::vector<pluss::Series>{v.unsqueeze(0)}, 0);  // [component, x, y]
  const auto energy = rho.div(0.4).add(v.square().mul(0.5));
  const auto state = gas::make(common::SeriesQuantity<gas::Density>::from_si(rho), common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                               common::SeriesQuantity<gas::EnergyDensity>::from_si(energy), grid, model);

  // the step limit has an acoustic part and a diffusive part; the smaller one rules
  out.writeln("shear viscosity   acoustic limit   diffusive limit   allowed step");
  for (double mu : {0.0, 0.002, 0.01, 0.05}) {
    gas::Transport transport;
    transport.shear = common::ScalarQuantity<gas::DynamicViscosity>::from_si(mu);
    const auto limits = gas::limits(state, grid, model, transport);
    out.writeln("  ", mu, "          ", limits.acoustic.si_value().item<double>(), "          ", limits.diffusion.si_value().item<double>(), "          ", limits.allowed.si_value().item<double>());
  }

}
```

Output

```text
shear viscosity   acoustic limit   diffusive limit   allowed step
  0.00000          0.01055          inf          0.01055
  0.00200          0.01055          0.18029          0.01055
  0.01000          0.01055          0.03606          0.01055
  0.05000          0.01055          0.00721          0.00721
```

Without viscosity there is no diffusive limit (it is infinite) and the acoustic step `0.01055` rules. As the viscosity rises the diffusive limit falls in inverse proportion, `0.18029`, `0.03606`, `0.00721`, and at `0.05` it falls below the acoustic one, so the allowed step drops to `0.00721`. A very viscous gas on a fine grid is limited by diffusion, not by sound waves.

## Decay of a shear wave

A shear wave of wavenumber {math}`k = 2\pi` in a uniform gas decays as {math}`e^{-\mu k^2 t/\rho}`; this is an exact solution of the linearised equations and a clean test of the viscous terms. The program runs the wave with shear viscosity 0.01 to `t = 0.5` and projects the final cross-stream momentum on the initial wave.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace gas = natsci::physics::fluids::gas;

static common::ScalarQuantity<common::Length> metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
static common::ScalarQuantity<common::Time> seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  gas::Model model;  // dimensionless gas, T = p / rho
  model.gas_constant = common::ScalarQuantity<gas::SpecificGasConstant>::from_si(1.0);

  // a shear wave in a periodic square of side 1: the cross-stream speed v = 0.01 sin(2 pi x) in a uniform gas of density 1 and pressure 1
  const std::int64_t n = 32, m = 4;
  const auto grid = spatial::make_periodic_grid({n, m}, {metres(1.0 / n), metres(1.0 / m)});
  const auto x = pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).div(static_cast<double>(n)).reshape({n, 1}).expand({n, m}, true);
  const auto rho = pluss::series::ones({n, m}, pluss::ScalarType::Float64);
  const auto v = x.mul(2.0 * std::numbers::pi).sin().mul(0.01);
  const auto momentum = pluss::series::zeros_like(v).unsqueeze(0).unite(std::vector<pluss::Series>{v.unsqueeze(0)}, 0);  // [component, x, y]
  const auto energy = rho.div(0.4).add(v.square().mul(0.5));
  const auto state = gas::make(common::SeriesQuantity<gas::Density>::from_si(rho), common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                               common::SeriesQuantity<gas::EnergyDensity>::from_si(energy), grid, model);

  // evolve with the viscosity switched on and compare the decay of the wave with exp(-mu k^2 t / rho)
  gas::RunOptions run;
  run.step.rule = gas::Rule::Rk3;
  run.step.flux = gas::FluxScheme::Hllc;
  run.step.recon = gas::Recon::Mc;
  const double mu = 0.01, stop = 0.5, k = 2.0 * std::numbers::pi;
  run.step.transport.shear = common::ScalarQuantity<gas::DynamicViscosity>::from_si(mu);
  const auto path = gas::evolve_to(state, grid, seconds(0.0), seconds(stop), model, run);
  const auto end_momentum = path.trajectory.state().get<gas::MomDensity>("momentum").si_value().select(0, -1);
  const double fraction = end_momentum.select(0, 1).mul(v).sum().item<double>() / v.square().sum().item<double>();  // projection of the cross-stream momentum on the first wave (density is 1)
  out.writeln("after t = ", stop, ": the wave has fallen to ", fraction, " of its first height; exact exp(-mu k^2 t) = ", std::exp(-mu * k * k * stop), " in ", path.steps, " steps");
}
```

Output

```text
after t = 0.50000: the wave has fallen to 0.82294 of its first height; exact exp(-mu k^2 t) = 0.82087 in 53 steps
```

The wave has fallen to `0.82294` of its first height; the exact factor {math}`e^{-0.01 \cdot 4\pi^2 \cdot 0.5} = 0.82087`. The difference of a quarter of a percent is the numerical error of a `32`-cell grid and the second-order scheme. The run took 53 steps, which you can compare with the stable step of `0.01055`: `0.5 / 0.01055 = 47` steps at the full limit, and `0.5 / (0.9 * 0.01055) = 53` at the safety factor of `0.9` that `evolve_to` applies.

## Sutherland's law for air

With `gas::transport` the viscosity and conductivity are fields that follow the local temperature. The program builds a uniform flow of air at four temperatures and 101325 Pa, asks for the Sutherland transport, and compares the viscosity with the formula typed out by hand. Then it builds a shear flow {math}`u = 20 \sin 2\pi y` m/s at 300 K and takes the stress tensor of it with `gas::stress`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace gas = natsci::physics::fluids::gas;

static common::ScalarQuantity<common::Length> metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }

// air at pressure p and temperature T, flowing at speed (u, v) in a periodic n x n square of side 1 metre
static gas::State air(const spatial::PeriodicGrid &grid, std::int64_t n, const pluss::Series &T, const pluss::Series &u, double p) {
  const gas::Model model;  // the defaults are air: gamma 1.4 and R = 287.05 J/(kg K)
  const auto rho = pluss::series::full_like(T, p).div(T.mul(model.gas_constant.si_value()));
  const auto momentum = rho.mul(u).unsqueeze(0).unite(std::vector<pluss::Series>{pluss::series::zeros_like(u).unsqueeze(0)}, 0);
  const auto energy = pluss::series::full_like(T, p / 0.4).add(rho.mul(u.square()).mul(0.5));
  return gas::make(common::SeriesQuantity<gas::Density>::from_si(rho), common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                   common::SeriesQuantity<gas::EnergyDensity>::from_si(energy), grid);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(4).with_sep(""));
  const std::int64_t n = 16;
  const auto grid = spatial::make_periodic_grid({n, n}, {metres(1.0 / n), metres(1.0 / n)});
  const gas::Model model;
  const gas::SutherlandLaw law;  // the defaults are air: mu = 1.716e-5 Pa s at 273.15 K, S = 110.4 K, Pr = 0.71
  const auto zero = pluss::series::zeros({n, n}, pluss::ScalarType::Float64);

  out.writeln("Sutherland's law for air at 101325 Pa: temperature, viscosity from the library, viscosity from the formula, conductivity");
  for (double T : {250.0, 300.0, 400.0, 800.0}) {
    const auto state = air(grid, n, pluss::series::full({n, n}, T, pluss::ScalarType::Float64), zero, 101325.0);
    const auto field = gas::transport(state, grid, model, law);
    const double mu = field.shear.si_value().select(0, 3).select(0, 3).item<double>();
    const double formula = 1.716e-5 * std::pow(T / 273.15, 1.5) * (273.15 + 110.4) / (T + 110.4);
    const double k = field.heat.si_value().select(0, 3).select(0, 3).item<double>();
    out.writeln("  ", T, " K  ", mu, "  ", formula, "  ", k, " W/(m K)");
  }

  // a shear flow u = 20 sin(2 pi y) m/s at 300 K: the shear stress is mu du/dy, largest where the flow is steepest
  const auto y = pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).div(static_cast<double>(n)).reshape({1, n}).expand({n, n}, true);
  const auto u = y.mul(2.0 * std::numbers::pi).sin().mul(20.0);
  const auto state = air(grid, n, pluss::series::full({n, n}, 300.0, pluss::ScalarType::Float64), u, 101325.0);
  const auto mu = gas::transport(state, grid, model, law).shear.si_value().select(0, 0).select(0, 0).item<double>();
  const auto tau = gas::stress(state, grid, gas::Transport{common::ScalarQuantity<gas::DynamicViscosity>::from_si(mu), common::ScalarQuantity<gas::DynamicViscosity>::from_si(0.0),
                                                           common::ScalarQuantity<gas::ThermalConductivity>::from_si(0.0)})
                       .si_value();
  out.writeln("stress tensor has shape ", tau.sizes());
  out.writeln("largest shear stress ", tau.abs().max().item<double>(), " Pa; the exact peak mu * 20 * 2 pi = ", mu * 20.0 * 2.0 * std::numbers::pi, " Pa");
  // the grid samples the wave at cell centres and differences it across two cells: that gives mu * 20 * cos(k y) * sin(k h) / h at the cell nearest the peak
  const double h = 1.0 / n, k = 2.0 * std::numbers::pi;
  out.writeln("what a central difference on this grid gives at the cell nearest the peak: ", mu * 20.0 * std::cos(k * 0.5 * h) * std::sin(k * h) / h, " Pa");
}
```

Output

```text
Sutherland's law for air at 101325 Pa: temperature, viscosity from the library, viscosity from the formula, conductivity
  2.5000e+02 K  1.5991e-05  1.5991e-05  2.2627e-02 W/(m K)
  3.0000e+02 K  1.8459e-05  1.8459e-05  2.6121e-02 W/(m K)
  4.0000e+02 K  2.2852e-05  2.2852e-05  3.2336e-02 W/(m K)
  8.0000e+02 K  3.6236e-05  3.6236e-05  5.1276e-02 W/(m K)
stress tensor has shape [2, 2, 16, 16]
largest shear stress 2.2171e-03 Pa; the exact peak mu * 20 * 2 pi = 2.3196e-03 Pa
what a central difference on this grid gives at the cell nearest the peak: 2.2171e-03 Pa
```

The library viscosity matches the formula to all printed digits at every temperature, `1.5991e-05` at 250 K rising to `3.6236e-05` at 800 K: a gas gets thicker as it gets hotter, the opposite of a liquid. The conductivity follows the viscosity through the Prandtl number, `2.6121e-02` W/(m K) at 300 K. The stress tensor has shape `[2, 2, 16, 16]` (two components by two components by the grid), and its largest entry `2.2171e-03` Pa is smaller than the exact peak {math}`\mu \cdot 20 \cdot 2\pi = 2.3196\text{e-}03` Pa by the amount a central difference on 16 cells gives, `mu * 20 * cos(k y) sin(k h)/h`, which the last line of the output evaluates: `2.2171e-03` Pa, the same number.

## Decisions

- Use constant `Transport` when the temperature range is small and `SutherlandLaw` when it is not; with Sutherland the coefficients are recomputed from the temperature of every cell at every stage.
- Set the bulk viscosity {math}`\zeta` to zero (the default) unless you know better; Stokes' hypothesis is a good model for monatomic and nearly-monatomic gases. The solver applies the {math}`-\tfrac23\mu\,\nabla\cdot u` term itself, so do not encode it in a negative coefficient.
- For a run with viscosity you do not have to change anything else: `evolve_to` reads the transport from the options and takes the smaller of the acoustic and diffusive limits.
- Resolve a boundary layer with the cells you can afford only after you have checked the Reynolds number: a viscous layer of thickness {math}`\delta \sim L/\sqrt{\mathrm{Re}}` needs several cells across it, and at high Reynolds number the turbulence closures of [Physics 7](../phys_07_les.md) and [Physics 8](../phys_08_rans.md) take over.

## Pitfalls

The step is checked against the diffusive limit as well as the acoustic one, and the transport coefficients are validated. A viscosity of `0.05` makes the diffusive limit the tighter one, and a negative viscosity is not a gas.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace common = natsci::common;
namespace spatial = common::spatial;
namespace gas = natsci::physics::fluids::gas;

static common::ScalarQuantity<common::Length> metres(double v) { return common::ScalarQuantity<common::Length>::from_si(v); }
static common::ScalarQuantity<common::Time> seconds(double v) { return common::ScalarQuantity<common::Time>::from_si(v); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  gas::Model model;  // dimensionless gas, T = p / rho
  model.gas_constant = common::ScalarQuantity<gas::SpecificGasConstant>::from_si(1.0);

  // a shear wave in a periodic square of side 1: the cross-stream speed v = 0.01 sin(2 pi x) in a uniform gas of density 1 and pressure 1
  const std::int64_t n = 32, m = 4;
  const auto grid = spatial::make_periodic_grid({n, m}, {metres(1.0 / n), metres(1.0 / m)});
  const auto x = pluss::series::arange(0, n, 1, pluss::ScalarType::Float64).add(0.5).div(static_cast<double>(n)).reshape({n, 1}).expand({n, m}, true);
  const auto rho = pluss::series::ones({n, m}, pluss::ScalarType::Float64);
  const auto v = x.mul(2.0 * std::numbers::pi).sin().mul(0.01);
  const auto momentum = pluss::series::zeros_like(v).unsqueeze(0).unite(std::vector<pluss::Series>{v.unsqueeze(0)}, 0);  // [component, x, y]
  const auto energy = rho.div(0.4).add(v.square().mul(0.5));
  const auto state = gas::make(common::SeriesQuantity<gas::Density>::from_si(rho), common::SeriesQuantity<gas::MomDensity>::from_si(momentum),
                               common::SeriesQuantity<gas::EnergyDensity>::from_si(energy), grid, model);

  // the step is checked against the diffusive limit too: a viscous gas needs a smaller step than the same gas without viscosity
  gas::Options options;
  options.transport.shear = common::ScalarQuantity<gas::DynamicViscosity>::from_si(0.05);
  const double allowed = gas::step_limit(state, grid, model, options.transport).si_value().min().item<double>();
  out.writeln("the allowed step with a viscosity of 0.05 is ", allowed, " s");
  try {
    (void)gas::step(state, grid, seconds(0.009), model, options);
  } catch (const pluss::Error &e) {
    out.writeln("a step of 0.009 s: ", e.message());
  }
  // a negative viscosity is not a gas: it is refused when the options are checked
  options.transport.shear = common::ScalarQuantity<gas::DynamicViscosity>::from_si(-0.01);
  try {
    (void)gas::step(state, grid, seconds(0.001), model, options);
  } catch (const pluss::Error &e) {
    out.writeln("a negative viscosity: ", e.message());
  }
}
```

Output

```text
the allowed step with a viscosity of 0.05 is 0.00721 s
a step of 0.009 s: natsci::fluids::gas::step: width exceeds the acoustic or diffusion stability limit
a negative viscosity: natsci::fluids::gas::step: transport coefficients must be finite and non-negative
```

The allowed step with a viscosity of `0.05` is `0.00721` s, so a step of `0.009` s, which would have been fine without viscosity (the acoustic limit is `0.01055` s), is refused with a message that names both limits. A negative viscosity is refused when the options are checked, with the message that the coefficients must be finite and non-negative. In both cases the call fails before any time stepping, so no partly advanced state is left behind.

## Where next

The next steps depend on what you simulate. For flows too slow to be compressible, the [incompressible solver](../phys_02_fluids/taylor_green_vortex.md) is cheaper. For turbulent flows at high Reynolds number, [Physics 7](../phys_07_les.md) closes the unresolved scales of a large-eddy simulation and [Physics 8](../phys_08_rans.md) models all of the turbulence.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="walls_inlets_outlets.html">
    <span class="label">Previous</span>
    <span class="title">Walls, inlets and outlets</span>
  </a>
</div>
<!-- apiref:nav:end -->
