<!-- generated from arteries.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Arteries

[Back to Biology 13: Blood Flow](../bio_13_blood_flow.md)

Blood flows through vessels that resist it, stretch with each beat and carry a pressure wave faster than the blood itself. This page computes the resistance of arteries of three sizes, how pulsatile the flow is in them, the speed of the pulse wave, and the impedance of a standard model of the arterial tree.

## How it works

`poiseuille_resistance(viscosity, length, radius)` is `8 viscosity length / (pi radius^4)`: the resistance of a long straight tube to steady flow, extremely sensitive to the radius. `womersley_number(radius, omega, density, viscosity)` compares the vessel radius with the distance viscosity can spread in one beat; below about `1` the flow follows the pressure instantly, above it the flow lags. `moens_korteweg(young, thickness, density, radius)` is the speed of the pressure wave in an elastic tube. `windkessel3_impedance(omega, characteristic, resistance, compliance)` is the impedance, at each angular frequency, of a three-element model in which a characteristic resistance sits in front of a compliance and a peripheral resistance in parallel.

## Three vessels

Blood has density `1060` kg/m³ and viscosity `3.5e-3` Pa s. The program prints the resistance of a `0.3` m artery of radius `2`, `4` and `8` mm, the Womersley numbers at a heart rate of `1.2` Hz, and the wave speeds for a wall `1` mm thick of modulus `0.4` MPa, then the impedance modulus of a Windkessel model at five frequencies.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace hemo = biology::biomechanics::hemodynamics;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // blood: density 1060 kg/m^3, viscosity 3.5e-3 Pa s. An artery 0.3 m long, radius 4 mm; resistance in Pa s / m^3
  const double density = 1060.0, viscosity = 3.5e-3;
  const auto radius = vec({0.002, 0.004, 0.008});
  out.writeln("radius (mm)                  ", flat(radius.mul(1000.0)));
  out.writeln("Poiseuille resistance (MPa s/m^3) ", flat(hemo::poiseuille_resistance(viscosity, 0.3, radius).mul(1e-6)));
  // the pulse repeats at 1.2 Hz: how oscillatory is the flow?
  const double omega = 2.0 * 3.141592653589793 * 1.2;
  out.writeln("Womersley number             ", flat(hemo::womersley_number(radius, vec({omega, omega, omega}), density, viscosity)));
  // speed of the pressure wave in an artery with a 1 mm wall of Young's modulus 0.4 MPa
  out.writeln("pulse-wave speed (m/s)       ", flat(hemo::moens_korteweg(4.0e5, 1.0e-3, density, radius)));

  // three-element Windkessel: characteristic 0.05, peripheral resistance 1.0 (both in 1e8 Pa s/m^3), compliance 1e-8 m^3/Pa
  const auto w = vec({0.0, 1.0, 5.0, 10.0, 30.0});
  const auto z = hemo::windkessel3_impedance(w, 0.05e8, 1.0e8, 1.0e-8);
  out.writeln("frequency (rad/s)            ", flat(w));
  out.writeln("impedance modulus (1e8)      ", flat(z.abs().mul(1e-8)));
}
```

Output

```text
radius (mm)                  [2.0000, 4.0000, 8.0000]
Poiseuille resistance (MPa s/m^3) [167.1127, 10.4445, 0.6528]
Womersley number             [3.0222, 6.0445, 12.0890]
pulse-wave speed (m/s)       [9.7129, 6.8680, 4.8564]
frequency (rad/s)            [0.0000, 1.0000, 5.0000, 10.0000, 30.0000]
impedance modulus (1e8)      [1.0500, 0.7433, 0.2117, 0.1157, 0.0610]
```

Resistance falls steeply with radius: `167.1127` MPa s/m³ at `2` mm, `10.4445` at `4` mm and `0.6528` at `8` mm, a factor of sixteen for each doubling of radius. The Womersley number doubles with the radius, from `3.02` to `12.09`, so the flow in all three vessels is oscillatory rather than quasi-steady. The pulse wave travels at `9.71`, `6.87` and `4.86` m/s, slower in the wider vessels. The Windkessel impedance starts at the sum of the two resistances, `1.05` in units of `1e8` Pa s/m³, at zero frequency and falls toward the characteristic resistance `0.05`, `0.0610` at `30` rad/s: the compliance absorbs the pulsatile part of the load.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
