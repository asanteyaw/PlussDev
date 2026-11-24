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

## The Windkessel model

The two-element Windkessel, a resistance in parallel with a compliance, has the impedance {math}`Z = R/(1 + i\omega RC)`. The three-element model adds a characteristic impedance in series, which stops the impedance from falling to zero at high frequency. The program prints the moduli and the phase of both at five frequencies and the decay of the pressure in diastole.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace hemo = biology::biomechanics::hemodynamics;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // impedance of the two-element and three-element Windkessel (R = 1e8, C = 1e-8, Zc = 0.05e8): modulus and phase at five frequencies
  const auto w = vec({0.5, 1.0, 5.0, 20.0, 100.0});
  const auto z2 = hemo::windkessel2_impedance(w, 1.0e8, 1.0e-8), z3 = hemo::windkessel3_impedance(w, 0.05e8, 1.0e8, 1.0e-8);
  out.writeln("frequency (rad/s)        ", flat(w));
  out.writeln("two-element modulus      ", flat(z2.abs().mul(1e-8)));
  out.writeln("three-element modulus    ", flat(z3.abs().mul(1e-8)));
  out.writeln("two-element phase (rad)  ", flat(z2.arg()));
  // the capacitor pressure during diastole: exact decay with the time constant R C = 1 s
  const auto pc = vec({100.0});
  const auto flow = vec({0.0});
  out.writeln("pressure after 0.5, 1, 2 s of diastole = ", std::vector<double>{flat(hemo::windkessel_capacitor_step(pc, flow, 0.5, 1.0e8, 1.0e-8))[0], flat(hemo::windkessel_capacitor_step(pc, flow, 1.0, 1.0e8, 1.0e-8))[0], flat(hemo::windkessel_capacitor_step(pc, flow, 2.0, 1.0e8, 1.0e-8))[0]});
}
```

Output

```text
frequency (rad/s)        [0.5000, 1.0000, 5.0000, 20.0000, 100.0000]
two-element modulus      [0.8944, 0.7071, 0.1961, 0.0499, 0.0100]
three-element modulus    [0.9394, 0.7433, 0.2117, 0.0724, 0.0511]
two-element phase (rad)  [-0.4636, -0.7854, -1.3734, -1.5208, -1.5608]
pressure after 0.5, 1, 2 s of diastole = [60.6531, 36.7879, 13.5335]
```

The two-element modulus falls from `0.8944` to `0.0100` over the frequencies while the three-element one levels off at `0.0511`, near the characteristic `0.05`. The phase tends to {math}`-\pi/2`, `-1.5608`, as the capacitor dominates. In diastole the pressure decays by the factor {math}`e^{-t/RC}`: from `100` to `60.6531`, `36.7879` and `13.5335` after `0.5`, `1` and `2` seconds.

## Pulsatile flow in a vessel

With the Womersley solution, the velocity amplitude, the flow and the wall shear of a pulsatile flow follow from the pressure gradient. `womersley_velocity` gives the profile, `womersley_flow` the flow and `womersley_wall_shear` the shear on the wall. The program uses a `4` mm artery at a heart rate of `1.2` Hz with a pressure-gradient amplitude of `100` Pa/m, and computes the wave speed from the pressure-area relation with `bramwell_hill`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace hemo = biology::biomechanics::hemodynamics;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // pulsatile flow in a 4 mm artery at a heart rate of 1.2 Hz, for blood (rho 1060, mu 3.5e-3): the velocity profile and the wall shear
  const double rho = 1060.0, mu = 3.5e-3, radius = 0.004, omega = 2.0 * 3.141592653589793 * 1.2, gradient = 100.0;
  const auto alpha = hemo::womersley_number(vec({radius}), vec({omega}), rho, mu);
  out.writeln("Womersley number = ", flat(alpha));
  const auto s = vec({0.0, 0.25, 0.5, 0.75, 0.95});
  const auto velocity = hemo::womersley_velocity(s, alpha.mul(0.0).add(flat(alpha)[0]), s.mul(0.0).add(omega), rho, s.mul(0.0).add(gradient));
  out.writeln("relative radius               ", flat(s));
  out.writeln("velocity amplitude (m/s)      ", flat(velocity.abs()));
  const auto flow = hemo::womersley_flow(vec({radius}), alpha, vec({omega}), rho, vec({gradient}));
  out.writeln("flow amplitude (mL/s)         ", flat(flow.abs().mul(1.0e6)));
  const auto shear = hemo::womersley_wall_shear(vec({radius}), alpha, vec({omega}), rho, mu, vec({gradient}));
  out.writeln("wall shear stress amplitude (Pa) = ", flat(shear.abs()));
  // pressure-wave speed from the pressure-area relation (Bramwell-Hill): area 5e-5 m^2, dA/dP = 1.2e-9 m^2/Pa
  out.writeln("wave speed (m/s) = ", flat(hemo::bramwell_hill(vec({5.0e-5}), rho, vec({1.2e-9}))));
}
```

Output

```text
Womersley number = [6.0445]
relative radius               [0.0000, 0.2500, 0.5000, 0.7500, 0.9500]
velocity amplitude (m/s)      [0.0133, 0.0136, 0.0138, 0.0110, 0.0032]
flow amplitude (mL/s)         [0.4982]
wall shear stress amplitude (Pa) = [0.0625]
wave speed (m/s) = [6.2696]
```

The Womersley number is `6.0445`, so the profile is flat rather than parabolic: the velocity amplitude is `0.0133` m/s on the axis, peaks at `0.0138` at half the radius and falls to `0.0032` near the wall. The flow amplitude is `0.4982` mL/s and the wall shear `0.0625` Pa. For an area of `5e-5` m² that changes by `1.2e-9` m² per pascal the pressure wave travels at `6.2696` m/s.

## Decisions

- Use `poiseuille_resistance` for steady flow in a long tube and `womersley_number` to decide whether steady flow is a fair approximation: above a Womersley number of about one it is not.
- Use `moens_korteweg` when you know the wall (modulus, thickness) and `bramwell_hill` when you know how the area responds to pressure.
- Use the two-element Windkessel when you only need the compliance and the resistance, and the three-element one when the high-frequency impedance must not fall to zero.

## Pitfalls

Every quantity is in SI units and the heart rate enters as an angular frequency. The program passes a radius in millimetres, a frequency in hertz and a modulus in megapascals as bare numbers.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace systems = biology::systems;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace hemo = biology::biomechanics::hemodynamics;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double pi = 3.141592653589793;
  // everything is in SI units: a radius of 4 (millimetres typed as a number) is four metres
  out.writeln("resistance for radius 0.004 m and 4 (mm by mistake): ", flat(hemo::poiseuille_resistance(3.5e-3, 0.3, vec({0.004, 4.0}))));
  // omega is in radians per second, not hertz
  out.writeln("Womersley number with 1.2 (Hz by mistake) and 2 pi 1.2 rad/s: ", flat(hemo::womersley_number(vec({0.004, 0.004}), vec({1.2, 2.0 * pi * 1.2}), 1060.0, 3.5e-3)));
  // Young's modulus is in pascals: 0.4 MPa is 4e5, and 0.4 typed as megapascals gives a wave speed about a thousand times too small
  out.writeln("pulse-wave speed for E = 0.4 MPa and for 0.4 (megapascals typed as a bare number): ", std::vector<double>{flat(hemo::moens_korteweg(4.0e5, 1.0e-3, 1060.0, vec({0.004})))[0], flat(hemo::moens_korteweg(0.4, 1.0e-3, 1060.0, vec({0.004})))[0]});
}
```

Output

```text
resistance for radius 0.004 m and 4 (mm by mistake): [10444543.1404, 0.0000]
Womersley number with 1.2 (Hz by mistake) and 2 pi 1.2 rad/s: [2.4114, 6.0445]
pulse-wave speed for E = 0.4 MPa and for 0.4 (megapascals typed as a bare number): [6.8680, 0.0069]
```

A radius of `4` instead of `0.004` m makes the resistance `0.0000` rather than `10444543.1404` Pa s/m³. A frequency of `1.2` instead of {math}`2\pi \times 1.2` rad/s gives a Womersley number of `2.4114` instead of `6.0445`. A modulus of `0.4` instead of `4e5` Pa gives a pulse speed of `0.0069` m/s instead of `6.8680`. None of them is an error, so convert to SI before the call.

## Where next

[Biology 18: Tissue Mechanics](../bio_18_tissue.md) gives the stress-strain behaviour of the wall that sets the pulse speed, and [Biology 11: Muscle](../bio_11_muscle.md) turns to the muscles that move the blood.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
