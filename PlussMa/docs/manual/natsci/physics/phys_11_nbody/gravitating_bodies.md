# Gravitating bodies

[Back to Physics 11: Gravitating Bodies](../phys_11_nbody.md)

Several masses attract each other and you want to follow them: three stars, a planet with its star, a cloud of particles. No formula gives their paths for three or more bodies, so you step them forward in time and check the quantities that must not change. This page describes a system of bodies, reads its energy, momentum and angular momentum, steps it with a time-reversible rule, runs many systems in one call, and applies your own force instead of gravity.

## How it works

`mechanics::make(mass, position, velocity)` builds the system from a mass of shape `[bodies]`, positions of shape `[..., bodies, axes]` and velocities of the same shape, with one to three axes. Any leading axes are separate systems, and everything below works on all of them at once. The pull on body {math}`i` is

```{math}
\mathbf a_i = G\sum_{j\ne i} m_j\,\frac{\mathbf r_j - \mathbf r_i}{\bigl(|\mathbf r_j - \mathbf r_i|^2 + \varepsilon^2\bigr)^{3/2}}
```

where {math}`\varepsilon` is an optional softening length that keeps a close pass finite. `mechanics::gravity(bodies, G, softening)` returns these accelerations; the constant defaults to the CODATA value of {math}`G`, so pass `G` explicitly when you work in other units. The conserved quantities are `total_mass`, `total_momentum`, `center` and `center_speed` of the centre of mass, `total_kinetic`, `potential_energy`, `total_energy` and, with three axes, `angular_momentum`. `mechanics::gravity_step(bodies, step, G, softening)` advances the system by one leapfrog step, the kick-drift-kick rule that is symplectic and reversible, so the energy error stays bounded instead of growing. `mechanics::leapfrog(bodies, step, acceleration)` is the same rule with an acceleration function of your own, and `shift_to_center` moves the system to the frame in which its centre of mass rests at the origin.

## A system and its conserved quantities

The figure-eight orbit is a famous solution of the three-body problem: three equal masses that chase each other along a single curve shaped like an eight. In units where {math}`G = 1` and every mass is one, the program builds it from its initial positions and velocities, prints every conserved quantity and the acceleration of each body.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace mechanics = natsci::physics::mechanics;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v, std::vector<std::int64_t> shape = {}) { auto s = shape.empty() ? pluss::series::make(v) : pluss::series::make(v, shape); return s.to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
using Mass = common::SeriesQuantity<common::Mass>;
using Len = common::SeriesQuantity<common::Length>;
using Spd = common::SeriesQuantity<common::Speed>;
// the figure-eight orbit of three equal masses, in units where G = 1 and every mass is 1
static mechanics::Bodies figure_eight() {
  return mechanics::make(Mass::from_si(vec({1.0, 1.0, 1.0})),
                         Len::from_si(vec({0.97000436, -0.24308753, 0.0, -0.97000436, 0.24308753, 0.0, 0.0, 0.0, 0.0}, {3, 3})),
                         Spd::from_si(vec({0.46620369, 0.43236573, 0.0, 0.46620369, 0.43236573, 0.0, -0.93240737, -0.86473146, 0.0}, {3, 3})));
}
static const auto G = common::ScalarQuantity<common::GravitationalConstant>::from_si(1.0);

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto bodies = figure_eight();
  out.writeln("bodies = ", bodies.count(), ", axes = ", bodies.axes());
  out.writeln("total mass         = ", flat(mechanics::total_mass(bodies).si_value()));
  out.writeln("total momentum     = ", flat(mechanics::total_momentum(bodies).si_value()));
  out.writeln("centre of mass     = ", flat(mechanics::center(bodies).si_value()));
  out.writeln("kinetic energy     = ", flat(mechanics::total_kinetic(bodies).si_value()));
  out.writeln("potential energy   = ", flat(mechanics::potential_energy(bodies, G).si_value()));
  out.writeln("total energy       = ", flat(mechanics::total_energy(bodies, G).si_value()));
  out.writeln("angular momentum   = ", flat(mechanics::angular_momentum(bodies).si_value()));
  // the acceleration of each body: the pull of the other two
  out.writeln("acceleration (one row per body):");
  const auto a = flat(mechanics::gravity(bodies, G).si_value());
  for (int i = 0; i < 3; ++i) out.writeln("  ", std::vector<double>{a[3 * i], a[3 * i + 1], a[3 * i + 2]});
}
```

Output

```text
bodies = 3, axes = 3
total mass         = [3.00000]
total momentum     = [0.00000, 0.00000, 0.00000]
centre of mass     = [0.00000, 0.00000, 0.00000]
kinetic energy     = [1.21286]
potential energy   = [-2.50000]
total energy       = [-1.28714]
angular momentum   = [0.00000, 0.00000, 0.00000]
acceleration (one row per body):
  [-1.21251, 0.30386, 0.00000]
  [1.21251, -0.30386, 0.00000]
  [0.00000, 0.00000, 0.00000]
```

The system has `3` bodies in `3` axes and a total mass of `3.00000`. Momentum and the centre of mass are zero, and so is the angular momentum, as for the figure-eight. The kinetic energy is `1.21286`, the potential energy `-2.50000` and the total energy `-1.28714`, which is negative: the system is bound. The accelerations of the first two bodies are equal and opposite, `(-1.21251, 0.30386, 0)` and `(1.21251, -0.30386, 0)`, and the third, at the centre, feels no net pull, as the symmetry demands.

## One period, and how the step matters

The orbit repeats with a period of `6.32591398`. The program steps it for exactly one period with `200`, `800` and `3200` steps and prints where the first body ends, against where it began, and the largest departure of the energy from its starting value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace mechanics = natsci::physics::mechanics;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v, std::vector<std::int64_t> shape = {}) { auto s = shape.empty() ? pluss::series::make(v) : pluss::series::make(v, shape); return s.to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
using Mass = common::SeriesQuantity<common::Mass>;
using Len = common::SeriesQuantity<common::Length>;
using Spd = common::SeriesQuantity<common::Speed>;
// the figure-eight orbit of three equal masses, in units where G = 1 and every mass is 1
static mechanics::Bodies figure_eight() {
  return mechanics::make(Mass::from_si(vec({1.0, 1.0, 1.0})),
                         Len::from_si(vec({0.97000436, -0.24308753, 0.0, -0.97000436, 0.24308753, 0.0, 0.0, 0.0, 0.0}, {3, 3})),
                         Spd::from_si(vec({0.46620369, 0.43236573, 0.0, 0.46620369, 0.43236573, 0.0, -0.93240737, -0.86473146, 0.0}, {3, 3})));
}
static const auto G = common::ScalarQuantity<common::GravitationalConstant>::from_si(1.0);

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const double period = 6.32591398;
  const double e0 = flat(mechanics::total_energy(figure_eight(), G).si_value())[0];
  for (int steps : {200, 800, 3200}) {
    auto bodies = figure_eight();
    const auto dt = common::ScalarQuantity<common::Time>::from_si(period / steps);
    double worst = 0.0;
    for (int i = 0; i < steps; ++i) {
      bodies = mechanics::gravity_step(bodies, dt, G);
      worst = std::max(worst, std::abs(flat(mechanics::total_energy(bodies, G).si_value())[0] - e0));
    }
    const auto first = flat(bodies.position().si_value());
    out.writeln(steps, " steps: body 1 after one period = ", std::vector<double>{first[0], first[1]}, " (started at 0.970004, -0.243088), largest energy error = ", worst);
  }
}
```

Output

```text
200 steps: body 1 after one period = [0.968713, -0.245240] (started at 0.970004, -0.243088), largest energy error = 0.000064
800 steps: body 1 after one period = [0.969923, -0.243223] (started at 0.970004, -0.243088), largest energy error = 0.000004
3200 steps: body 1 after one period = [0.969999, -0.243096] (started at 0.970004, -0.243088), largest energy error = 0.000000
```

With `200` steps the first body ends about `0.0025` from where it began, and the energy strays by `0.000064` at most. Quadrupling the number of steps cuts both errors by a factor of `16`: the energy error falls to `0.000004` and the end point is `0.0002` away, at `0.969923, -0.243223`. At `3200` steps the body is back at `0.969999, -0.243096` and the energy error is below the six digits printed. This is the second-order accuracy of the leapfrog step: a step four times shorter has an error sixteen times smaller, and the energy error oscillates instead of drifting.

## Many systems and real units

Three Sun-Earth systems are advanced at once, in kilograms, metres and seconds. In each the Earth starts `1` au from the Sun and moves sideways at `0.8`, `1.0` and `1.5` times its circular speed. The program shifts to the centre-of-mass frame, steps `100` days with one-hour steps and prints the energy of each system and the separation of the bodies at the end.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace mechanics = natsci::physics::mechanics;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v, std::vector<std::int64_t> shape = {}) { auto s = shape.empty() ? pluss::series::make(v) : pluss::series::make(v, shape); return s.to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
using Mass = common::SeriesQuantity<common::Mass>;
using Len = common::SeriesQuantity<common::Length>;
using Spd = common::SeriesQuantity<common::Speed>;
// the figure-eight orbit of three equal masses, in units where G = 1 and every mass is 1
static mechanics::Bodies figure_eight() {
  return mechanics::make(Mass::from_si(vec({1.0, 1.0, 1.0})),
                         Len::from_si(vec({0.97000436, -0.24308753, 0.0, -0.97000436, 0.24308753, 0.0, 0.0, 0.0, 0.0}, {3, 3})),
                         Spd::from_si(vec({0.46620369, 0.43236573, 0.0, 0.46620369, 0.43236573, 0.0, -0.93240737, -0.86473146, 0.0}, {3, 3})));
}
static const auto G = common::ScalarQuantity<common::GravitationalConstant>::from_si(1.0);

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // three Sun-Earth systems at once: the Earth leaves a point 1 au from the Sun with 0.8, 1.0 and 1.5 times its circular speed
  const double au = 1.495978707e11, v0 = 29780.0;
  const auto sun_and_earth = Mass::from_si(vec({1.989e30, 5.972e24}));
  std::vector<double> x, v;
  for (double factor : {0.8, 1.0, 1.5}) {
    x.insert(x.end(), {0.0, 0.0, 0.0, au, 0.0, 0.0});
    v.insert(v.end(), {0.0, 0.0, 0.0, 0.0, factor * v0, 0.0});
  }
  auto bodies = mechanics::shift_to_center(mechanics::make(sun_and_earth, Len::from_si(vec(x, {3, 2, 3})), Spd::from_si(vec(v, {3, 2, 3}))));
  out.writeln("energy per system (J)      = ", flat(mechanics::total_energy(bodies).si_value().div(1e33)), " x 1e33");
  const auto day = common::ScalarQuantity<common::Time>::from_si(3600.0);
  for (int i = 0; i < 100 * 24; ++i) bodies = mechanics::gravity_step(bodies, day);
  const auto separation = bodies.position().si_value().select(-2, 1).sub(bodies.position().si_value().select(-2, 0)).norm(2.0, -1, false);
  out.writeln("separation after 100 days (au) = ", flat(separation.div(au)));
  out.writeln("energy per system now (J)  = ", flat(mechanics::total_energy(bodies).si_value().div(1e33)), " x 1e33");
}
```

Output

```text
energy per system (J)      = [-3.6047, -2.6514, 0.6588] x 1e33
separation after 100 days (au) = [0.5197, 0.9993, 2.0540]
energy per system now (J)  = [-3.6047, -2.6514, 0.6588] x 1e33
```

The energies, `-3.6047`, `-2.6514` and `0.6588` times {math}`10^{33}` J, are negative for the two slower launches (bound orbits) and positive for the fast one, which is above escape speed. After `100` days the slow Earth has fallen to `0.5197` au, the circular one stays at `0.9993` au, and the fast one has flown out to `2.0540` au. The energy of every system is unchanged to the digits printed, after `2400` steps.

## A force of your own

`leapfrog` takes any function that returns an acceleration. The program holds a unit mass with a spring, {math}`a = -\omega^2 x` with {math}`\omega = 2`, releases it from rest at {math}`x = 1` and steps it to a quarter of a period, {math}`\pi/4 \approx 0.785`, where it should be at the centre moving at speed {math}`\omega`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace mechanics = natsci::physics::mechanics;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v, std::vector<std::int64_t> shape = {}) { auto s = shape.empty() ? pluss::series::make(v) : pluss::series::make(v, shape); return s.to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
using Mass = common::SeriesQuantity<common::Mass>;
using Len = common::SeriesQuantity<common::Length>;
using Spd = common::SeriesQuantity<common::Speed>;
// the figure-eight orbit of three equal masses, in units where G = 1 and every mass is 1
static mechanics::Bodies figure_eight() {
  return mechanics::make(Mass::from_si(vec({1.0, 1.0, 1.0})),
                         Len::from_si(vec({0.97000436, -0.24308753, 0.0, -0.97000436, 0.24308753, 0.0, 0.0, 0.0, 0.0}, {3, 3})),
                         Spd::from_si(vec({0.46620369, 0.43236573, 0.0, 0.46620369, 0.43236573, 0.0, -0.93240737, -0.86473146, 0.0}, {3, 3})));
}
static const auto G = common::ScalarQuantity<common::GravitationalConstant>::from_si(1.0);

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // your own force: a unit-mass particle held by a spring toward the origin, a = -omega^2 x, with omega = 2
  auto particle = mechanics::make(Mass::from_si(vec({1.0})), Len::from_si(vec({1.0, 0.0, 0.0}, {1, 3})), Spd::from_si(vec({0.0, 0.0, 0.0}, {1, 3})));
  const auto spring = [](const mechanics::Bodies &b) {
    return common::SeriesQuantity<common::Acceleration>::from_si(b.position().si_value().mul(-4.0));
  };
  const auto dt = common::ScalarQuantity<common::Time>::from_si(0.001);
  const double e0 = flat(mechanics::kinetic_energy(particle).si_value())[0] + 0.5 * 4.0 * 1.0;
  // after a quarter period, pi/4, the particle has reached the centre moving at speed omega
  for (int i = 0; i < 785; ++i) particle = mechanics::leapfrog(particle, dt, spring);
  out.writeln("position after 0.785 = ", flat(particle.position().si_value()));
  out.writeln("velocity after 0.785 = ", flat(particle.velocity().si_value()));
  const double x = flat(particle.position().si_value())[0];
  out.writeln("energy: start ", e0, ", now ", flat(mechanics::total_kinetic(particle).si_value())[0] + 0.5 * 4.0 * x * x);
}
```

Output

```text
position after 0.785 = [0.000796, 0.000000, 0.000000]
velocity after 0.785 = [-2.000000, 0.000000, 0.000000]
energy: start 2.000000, now 2.000002
```

After `0.785` the position is `0.000796` and the velocity `-2.000000`, the particle at the centre moving at speed two, as the exact solution says. The energy, kinetic plus {math}`\tfrac12\omega^2 x^2`, is `2.000000` at the start and `2.000002` at the end.

## Decisions

- Work in units where {math}`G = 1` for a model system, and in SI units, with the default constant, for a real one. Mixing them is the usual source of a wrong period.
- Choose the step from the fastest motion in the system, the closest pair or the shortest period, not from the total time. Halving the step cuts the error by four, so test two steps and compare the energy.
- Add a softening length when bodies can pass very close to each other, and leave it at zero otherwise: softening changes the force law and the potential energy together, so the energy is conserved either way.
- Put separate systems on a leading axis to run them together, and build a custom force with `leapfrog` when gravity is only one part of it. The acceleration function must return a `SeriesQuantity<Acceleration>` of the same shape as the positions.

## Pitfalls

Bad systems are rejected when you build them, and a bad step when you take it. The program also shows what a step that is too long does to a pair of bodies in a bound orbit, in units where {math}`G = 1` and the pair has total energy `-0.91`. It spells the namespace out in full.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace mechanics = natsci::physics::mechanics;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v, std::vector<std::int64_t> shape = {}) { auto s = shape.empty() ? pluss::series::make(v) : pluss::series::make(v, shape); return s.to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
using Mass = common::SeriesQuantity<common::Mass>;
using Len = common::SeriesQuantity<common::Length>;
using Spd = common::SeriesQuantity<common::Speed>;
// the figure-eight orbit of three equal masses, in units where G = 1 and every mass is 1
static mechanics::Bodies figure_eight() {
  return pluss::ma::natsci::physics::mechanics::make(Mass::from_si(vec({1.0, 1.0, 1.0})),
                         Len::from_si(vec({0.97000436, -0.24308753, 0.0, -0.97000436, 0.24308753, 0.0, 0.0, 0.0, 0.0}, {3, 3})),
                         Spd::from_si(vec({0.46620369, 0.43236573, 0.0, 0.46620369, 0.43236573, 0.0, -0.93240737, -0.86473146, 0.0}, {3, 3})));
}
static const auto G = common::ScalarQuantity<common::GravitationalConstant>::from_si(1.0);

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto step = common::ScalarQuantity<common::Time>::from_si(0.001);
  // a body with a negative mass, and bodies with no mass at all
  try { (void)pluss::ma::natsci::physics::mechanics::make(Mass::from_si(vec({1.0, -1.0})), Len::from_si(vec({0, 0, 0, 1, 0, 0}, {2, 3})), Spd::from_si(vec({0, 0, 0, 0, 1, 0}, {2, 3}))); }
  catch (const pluss::Error &e) { out.writeln("negative mass: ", e.message()); }
  try { (void)pluss::ma::natsci::physics::mechanics::make(Mass::from_si(vec({0.0, 0.0})), Len::from_si(vec({0, 0, 0, 1, 0, 0}, {2, 3})), Spd::from_si(vec({0, 0, 0, 0, 1, 0}, {2, 3}))); }
  catch (const pluss::Error &e) { out.writeln("no mass: ", e.message()); }
  // positions in two dimensions: angular momentum needs three axes
  const auto flat_bodies = pluss::ma::natsci::physics::mechanics::make(Mass::from_si(vec({1.0, 1.0})), Len::from_si(vec({0, 0, 1, 0}, {2, 2})), Spd::from_si(vec({0, 0, 0, 1}, {2, 2})));
  try { (void)pluss::ma::natsci::physics::mechanics::angular_momentum(flat_bodies); }
  catch (const pluss::Error &e) { out.writeln("two axes: ", e.message()); }
  // a step that is zero, and a negative softening length
  try { (void)pluss::ma::natsci::physics::mechanics::gravity_step(flat_bodies, common::ScalarQuantity<common::Time>::from_si(0.0), G); }
  catch (const pluss::Error &e) { out.writeln("zero step: ", e.message()); }
  try { (void)pluss::ma::natsci::physics::mechanics::gravity(flat_bodies, G, common::ScalarQuantity<common::Length>::from_si(-0.1)); }
  catch (const pluss::Error &e) { out.writeln("negative softening: ", e.message()); }
  // a step that is too large for a close pair: energy grows instead of staying put
  auto pair = pluss::ma::natsci::physics::mechanics::make(Mass::from_si(vec({1.0, 1.0})), Len::from_si(vec({-0.5, 0, 0, 0.5, 0, 0}, {2, 3})), Spd::from_si(vec({0, -0.3, 0, 0, 0.3, 0}, {2, 3})));
  const double e0 = flat(pluss::ma::natsci::physics::mechanics::total_energy(pair, G).si_value())[0];
  for (double dt : {0.001, 0.05, 0.25}) {
    auto b = pair;
    for (int i = 0; i < static_cast<int>(40.0 / dt); ++i) b = pluss::ma::natsci::physics::mechanics::gravity_step(b, common::ScalarQuantity<common::Time>::from_si(dt), G);
    out.writeln("step ", dt, ": energy changed from ", e0, " to ", flat(pluss::ma::natsci::physics::mechanics::total_energy(b, G).si_value())[0]);
  }
}
```

Output

```text
negative mass: natsci::mechanics::make: mass must be non-negative
no mass: natsci::mechanics::make: every batch must have positive total mass
two axes: natsci::mechanics::angular_momentum: expected exactly three axes
zero step: natsci::mechanics::leapfrog: step must be finite and positive
negative softening: natsci::mechanics: softening must be finite and non-negative
step 0.0010: energy changed from -0.9100 to -0.9100
step 0.0500: energy changed from -0.9100 to 3.9868
step 0.2500: energy changed from -0.9100 to 7.9457
```

A negative mass and a system with no mass at all are refused by `make`, angular momentum needs `3` axes, a step of zero is refused and so is a negative softening. The step size is the mistake that raises no error: the pair started with an energy of `-0.9100` and with a step of `0.001` it ends there. With `0.05` its energy has risen to `3.9868`, which is positive, so the bodies have been thrown apart, and with `0.25` to `7.9457`. A bound orbit that comes apart is a sign of a step that is too long near the closest approach. Always print the energy of a run once.

## Where next

[Physics 5: Mechanics](../phys_05_mechanics.md) integrates one particle with Hamilton's equations and the same kind of symplectic step, and [Physics 1: Orbits](../phys_01_orbits.md) solves the two-body problem exactly, which is the check to use on a pair of bodies here.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
