# Lambert transfers

[Back to Physics 21: Orbit Transfers](../phys_21_transfers.md)

A spacecraft is at one point, a target will be at another point an hour later, and you want the velocity to leave with. This is Lambert's problem, the basic question of orbit transfer and of orbit determination from two sightings. This page solves it for one transfer and for many flight times at once, prograde and retrograde, in and out of the plane, and checks each answer by flying it.

## How it works

Given the starting position {math}`\mathbf r_1`, the final position {math}`\mathbf r_2`, the flight time {math}`\Delta t` and the gravitational parameter {math}`\mu`, there is a unique conic orbit through both points that takes exactly {math}`\Delta t`. Its geometry is set by the chord {math}`c = |\mathbf r_2 - \mathbf r_1|` and the semi-perimeter {math}`s = (r_1 + r_2 + c)/2`, and Lambert's equation links them to the flight time,

```{math}
\sqrt{\mu}\,\Delta t = a^{3/2}\bigl[(\alpha - \sin\alpha) - (\beta - \sin\beta)\bigr]
```

with {math}`\alpha` and {math}`\beta` angles that depend on {math}`a`, {math}`s` and {math}`c`. `astro::lambert(start, finish, flight, mu)` solves it by Householder iteration and returns the departure velocity `depart`, the arrival velocity `arrive`, the root found, the residual, and whether the iteration `converged`. Positions and the arrival and departure velocities are `[..., 3]` arrays, the flight time is one number or one number for each transfer, and `LambertOptions` chooses the sense, `OrbitSense::Prograde` (counter-clockwise about the {math}`+z` axis, the default) or `OrbitSense::Retrograde`. For endpoints that are not in the plane perpendicular to {math}`z`, pass a reference normal as a fifth argument: the transfer then goes around in the sense that has a positive component along it.

## One transfer around the Earth

The program takes two points around the Earth about an hour apart, solves for the velocities, and checks the answer by flying the departure state for the flight time with the Kepler propagator of Physics 1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
using Len = common::SeriesQuantity<common::Length>;
using Spd = common::SeriesQuantity<common::Speed>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double km = 1000.0;
  const auto mu = common::ScalarQuantity<astro::GravParam>::from_si(398600.0 * km * km * km);
  // two points on an orbit around the Earth and one hour to get from the first to the second
  const auto start = Len::from_si(vec({5000.0 * km, 10000.0 * km, 2100.0 * km}));
  const auto finish = Len::from_si(vec({-14600.0 * km, 2500.0 * km, 7000.0 * km}));
  const auto flight = common::ScalarQuantity<common::Time>::from_si(3600.0);
  const auto transfer = astro::lambert(start, finish, flight, mu);
  out.writeln("departure velocity (km/s) = ", flat(transfer.depart.si_value().div(km)));
  out.writeln("arrival velocity (km/s)   = ", flat(transfer.arrive.si_value().div(km)));
  out.writeln("iterations = ", transfer.steps, ", residual = ", flat(transfer.residual), ", converged = ", transfer.converged.item<bool>() ? "yes" : "no");
  // check by flying the departure state for one hour with the Kepler propagator
  const auto reached = astro::advance(astro::make_state(start, transfer.depart), flight, mu);
  out.writeln("distance from the requested end point (m) = ", flat(reached.state.position().si_value().sub(finish.si_value()).norm(2.0, -1, false)));
  out.writeln("arrival velocity mismatch (m/s)           = ", flat(reached.state.velocity().si_value().sub(transfer.arrive.si_value()).norm(2.0, -1, false)));
}
```

Output

```text
departure velocity (km/s) = [-5.9925, 1.9254, 3.2456]
arrival velocity (km/s)   = [-3.3125, -4.1966, -0.3853]
iterations = 3, residual = [0.0000], converged = yes
distance from the requested end point (m) = [0.0000]
arrival velocity mismatch (m/s)           = [0.0000]
```

The spacecraft must leave with `(-5.9925, 1.9254, 3.2456)` km/s and arrives with `(-3.3125, -4.1966, -0.3853)` km/s. The solver needed `3` iterations. Flying the departure state for the hour puts the spacecraft within `0.0000` m of the requested point, and the velocity it arrives with differs from the reported arrival velocity by `0.0000` m/s: the answer is a real orbit, not only a solution of an equation.

## Many flight times in one call

A spacecraft on a circular orbit of radius `7000` km must reach a point at radius `20000` km, `120` degrees ahead in the same plane. The program solves the problem for five flight times at once, by repeating the same endpoints and giving five different times, and prints the speed required and the velocity change from the circular orbit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
using Len = common::SeriesQuantity<common::Length>;
using Spd = common::SeriesQuantity<common::Speed>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double km = 1000.0, deg = std::numbers::pi / 180.0;
  const auto mu = common::ScalarQuantity<astro::GravParam>::from_si(398600.4418 * km * km * km);
  // a spacecraft on a circular orbit of radius 7000 km must reach a point at radius 20000 km, 120 degrees ahead in the same plane
  const double r1 = 7000.0 * km, r2 = 20000.0 * km, angle = 120.0 * deg;
  const std::vector<double> minutes = {40.0, 60.0, 90.0, 150.0, 300.0};
  const std::int64_t n = static_cast<std::int64_t>(minutes.size());
  std::vector<double> a, b, t;
  for (double m : minutes) {
    a.insert(a.end(), {r1, 0.0, 0.0});
    b.insert(b.end(), {r2 * std::cos(angle), r2 * std::sin(angle), 0.0});
    t.push_back(m * 60.0);
  }
  // one call solves all five flight times
  const auto transfer = astro::lambert(Len::from_si(pluss::series::make(a, {n, 3})), Len::from_si(pluss::series::make(b, {n, 3})),
                                       common::SeriesQuantity<common::Time>::from_si(vec(t)), mu);
  const double circular = std::sqrt(398600.4418 * km * km * km / r1);
  const auto speed = transfer.depart.si_value().norm(2.0, -1, false);
  const auto burn = transfer.depart.si_value().sub(vec({0.0, circular, 0.0})).norm(2.0, -1, false);
  out.writeln("flight time (min)       = ", flat(vec(minutes)));
  out.writeln("departure speed (km/s)  = ", flat(speed.div(km)));
  out.writeln("velocity change (km/s)  = ", flat(burn.div(km)));
  out.writeln("converged               = ", flat(transfer.converged.to(pluss::ScalarType::Float64)));
  out.writeln("circular speed at 7000 km (km/s) = ", circular / km);
}
```

Output

```text
flight time (min)       = [40.0000, 60.0000, 90.0000, 150.0000, 300.0000]
departure speed (km/s)  = [12.0787, 9.9797, 9.2067, 9.1427, 9.5215]
velocity change (km/s)  = [5.1384, 2.4348, 2.7002, 4.3179, 5.9325]
converged               = [1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
circular speed at 7000 km (km/s) = 7.5461
```

The five transfers all converge. A transfer in `40` minutes needs `12.0787` km/s, a velocity change of `5.1384` km/s from the `7.5461` km/s circular speed; the faster the transfer the more it costs. The cheapest of the five is the `60`-minute transfer, with a velocity change of `2.4348` km/s. Longer flights cost more again (`2.7002`, `4.3179` and `5.9325` km/s for `90`, `150` and `300` minutes), because a slow transfer has to leave on a path that wanders far from the target before it returns, and that takes a larger burn than a transfer of moderate length. The flight time is a design choice, and the cost has a minimum.

## Direction and plane

In units where {math}`\mu = 1` a circular orbit of radius one has speed one. The program makes a quarter turn counter-clockwise, then the three-quarter turn clockwise to the same point, and then a transfer to a point above the plane, where a reference normal has to say which way round to go.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
using Len = common::SeriesQuantity<common::Length>;
using Spd = common::SeriesQuantity<common::Speed>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // canonical units: mu = 1, so a circular orbit of radius 1 has speed 1 and period 2 pi
  const auto mu = common::ScalarQuantity<astro::GravParam>::from_si(1.0);
  const auto start = Len::from_si(vec({1.0, 0.0, 0.0}));
  const auto finish = Len::from_si(vec({0.0, 1.0, 0.0}));
  const auto quarter = common::ScalarQuantity<common::Time>::from_si(std::numbers::pi / 2.0);
  // a quarter of a circle counter-clockwise takes a quarter of the period
  const auto pro = astro::lambert(start, finish, quarter, mu);
  out.writeln("prograde, pi/2:   depart = ", flat(pro.depart.si_value()), ", arrive = ", flat(pro.arrive.si_value()));
  // the long way round, clockwise, to the same point takes three quarters of the period
  astro::LambertOptions clockwise;
  clockwise.sense = astro::OrbitSense::Retrograde;
  const auto retro = astro::lambert(start, finish, common::ScalarQuantity<common::Time>::from_si(3.0 * std::numbers::pi / 2.0), mu, clockwise);
  out.writeln("retrograde, 3pi/2: depart = ", flat(retro.depart.si_value()), ", arrive = ", flat(retro.arrive.si_value()));
  out.writeln("angular momentum, z component: prograde ", flat(start.si_value().cross(pro.depart.si_value(), -1))[2], ", retrograde ", flat(start.si_value().cross(retro.depart.si_value(), -1))[2]);
  // a target out of the plane: the sense is then decided by a reference normal, here pointing along -y
  const auto up = Len::from_si(vec({0.0, 0.0, 1.0}));
  const auto normal = common::SeriesQuantity<common::Dimensionless>::from_si(vec({0.0, -1.0, 0.0}));
  const auto tilted = astro::lambert(start, up, quarter, mu, normal);
  out.writeln("out of the plane: depart = ", flat(tilted.depart.si_value()), ", arrive = ", flat(tilted.arrive.si_value()));
}
```

Output

```text
prograde, pi/2:   depart = [-0.0000, 1.0000, 0.0000], arrive = [-1.0000, 0.0000, 0.0000]
retrograde, 3pi/2: depart = [0.0000, -1.0000, 0.0000], arrive = [1.0000, -0.0000, -0.0000]
angular momentum, z component: prograde 1.0000, retrograde -1.0000
out of the plane: depart = [-0.0000, 0.0000, 1.0000], arrive = [-1.0000, 0.0000, 0.0000]
```

The counter-clockwise quarter turn leaves at `(0, 1, 0)` and arrives at `(-1, 0, 0)`, the circular orbit's velocity at both ends. The clockwise transfer to the same point takes three quarters of a turn, leaves at `(0, -1, 0)` and has angular momentum `-1.0000` where the first has `1.0000`. For the point out of the plane, the normal `(0, -1, 0)` fixes the sense and the answer is a quarter turn that leaves along `z` and arrives along `-x`. The `-0.0000` entries are zeros of rounding noise.

## Decisions

- Give the five-argument form with a reference normal whenever the two endpoints are not in the equatorial plane or you want to control the sense; with only four arguments the sense follows `LambertOptions::sense` about {math}`+z`.
- Solve many transfers with one call: positions of shape `[n, 3]` and a time array of shape `[n]`, as the second example does, cost one iteration loop, not `n`.
- Check an answer the first time with `astro::advance` on the state you built from it, as the first example does. A transfer that converged but is not the one you meant, for example the wrong sense, is easy to see this way.
- Keep `strict = true` for work that uses the result and set `strict = false` only to look at a case that does not converge. The solver can take a tolerance and a step budget in `LambertOptions`.

## Pitfalls

Four inputs that Lambert's problem cannot answer, and a solver that is given too few steps. The program tries each.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
using Len = common::SeriesQuantity<common::Length>;
using Spd = common::SeriesQuantity<common::Speed>;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto mu = common::ScalarQuantity<astro::GravParam>::from_si(1.0);
  const auto start = Len::from_si(vec({1.0, 0.0, 0.0}));
  // an exactly opposite point: every plane through the two points fits
  try { (void)astro::lambert(start, Len::from_si(vec({-2.0, 0.0, 0.0})), common::ScalarQuantity<common::Time>::from_si(3.0), mu); }
  catch (const pluss::Error &e) { out.writeln("opposite points: ", e.message()); }
  // a flight time of zero
  try { (void)astro::lambert(start, Len::from_si(vec({0.0, 1.0, 0.0})), common::ScalarQuantity<common::Time>::from_si(0.0), mu); }
  catch (const pluss::Error &e) { out.writeln("zero flight time: ", e.message()); }
  // a normal that lies in the plane of the two points cannot decide the sense
  try { (void)astro::lambert(start, Len::from_si(vec({0.0, 1.0, 0.0})), common::ScalarQuantity<common::Time>::from_si(1.0), mu,
                             common::SeriesQuantity<common::Dimensionless>::from_si(vec({1.0, 0.0, 0.0}))); }
  catch (const pluss::Error &e) { out.writeln("normal in the plane: ", e.message()); }
  // a solver that is given too few iterations says so instead of returning a wrong answer
  astro::LambertOptions few;
  few.max_steps = 1;
  few.tolerance = 1e-15;
  try { (void)astro::lambert(start, Len::from_si(vec({0.0, 1.0, 0.0})), common::ScalarQuantity<common::Time>::from_si(3.0), mu, few); }
  catch (const pluss::Error &e) { out.writeln("one iteration: ", e.message()); }
  few.strict = false;
  const auto lax = astro::lambert(start, Len::from_si(vec({0.0, 1.0, 0.0})), common::ScalarQuantity<common::Time>::from_si(3.0), mu, few);
  out.writeln("strict = false: converged = ", lax.converged.item<bool>() ? "yes" : "no", ", iterations used = ", lax.steps);
}
```

Output

```text
opposite points: natsci::astro::lambert: collinear endpoints do not define a unique transfer plane
zero flight time: natsci::astro::lambert: flight time must be positive
normal in the plane: natsci::astro::lambert: reference normal must distinguish the transfer sense
one iteration: natsci::astro::lambert: transfer-time solve did not converge
strict = false: converged = no, iterations used = 1
```

Two points on opposite sides of the primary (a transfer of exactly `180` degrees) do not define a plane, so the solver refuses them rather than choosing one; give a transfer a little off `180` degrees or supply the plane some other way. A flight time of zero and a reference normal that lies in the plane of the two points, which cannot say which way round, are also refused. Too few iterations raise an error; with `strict = false` the call returns with `converged = no` after `1` iteration. Do not use that result as a transfer.

## Where next

[Physics 20: Kepler's Equation](../phys_20_kepler_equation.md) explains the propagation that the check relies on, and [Physics 1: Orbits](../phys_01_orbits.md) converts the departure state into orbital elements, from which `period`, `peri` and `apo` describe the transfer orbit.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
