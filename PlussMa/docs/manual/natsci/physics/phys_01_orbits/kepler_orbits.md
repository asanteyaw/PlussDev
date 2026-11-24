# Kepler orbits

[Back to Physics 1: Orbits](../phys_01_orbits.md)

A satellite moves along an ellipse around the body it orbits. Two ways describe the same orbit: its position and velocity at one instant, and six numbers that give the shape and orientation of the ellipse. This page converts between the two, reads the period and the closest and farthest distances, flies the satellite forward in time, and checks that the energy stays constant and that one period brings it back.

## How it works

Quantities carry their unit: `common::SeriesQuantity<common::Length>::from_si(x)` is a length given in metres, and the gravitational parameter of the central body is a `common::ScalarQuantity<astro::GravParam>`. `astro::make_state(position, velocity)` makes an orbit from the instantaneous state; `astro::to_elem(state, mu)` turns it into orbital elements, of which `axis()`, `ecc()` and `tilt()` are the semi-major axis, eccentricity and inclination. `astro::period`, `astro::peri` and `astro::apo` return the orbital period and the closest and farthest distances, and `astro::energy` the specific energy, which does not change along a Kepler orbit. `astro::advance(state, elapsed, mu)` moves the state forward by the elapsed time, exactly, by solving Kepler's equation instead of stepping.

## A satellite around the Earth

The satellite starts `7000` km from the centre of the Earth with a velocity of `7.5` km/s sideways and `1` km/s along the axis. The program prints the elements, the period and the extremes of the orbit, and then advances the satellite by a quarter, a half and a whole period.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;

static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double km = 1000.0;
  const auto mu = common::ScalarQuantity<astro::GravParam>::from_si(3.986004418e14);   // Earth, m^3/s^2

  // a satellite 7000 km from the centre of the Earth, moving at 7.5 km/s mostly sideways
  const auto start = astro::make_state(
      common::SeriesQuantity<common::Length>::from_si(pluss::series::make({7000.0 * km, 0.0, 0.0})),
      common::SeriesQuantity<common::Speed>::from_si(pluss::series::make({0.0, 7.5 * km, 1.0 * km})));

  const auto elements = astro::to_elem(start, mu);
  out.writeln("semi-major axis (km)  = ", flat(elements.axis().si_value().div(km)));
  out.writeln("eccentricity          = ", flat(elements.ecc().si_value()));
  out.writeln("inclination (deg)     = ", flat(elements.tilt().si_value().mul(180.0 / 3.141592653589793)));
  out.writeln("period (min)          = ", flat(astro::period(elements, mu).si_value().div(60.0)));
  out.writeln("closest approach (km) = ", flat(astro::peri(elements).si_value().div(km)));
  out.writeln("farthest point (km)   = ", flat(astro::apo(elements).si_value().div(km)));
  out.writeln("energy (MJ/kg)        = ", flat(astro::energy(start, mu).si_value().div(1e6)));

  // fly the satellite for a quarter, half and one full period
  const double period = flat(astro::period(elements, mu).si_value())[0];
  for (double fraction : {0.25, 0.5, 1.0}) {
    const auto later = astro::advance(start, common::ScalarQuantity<common::Time>::from_si(period * fraction), mu);
    const auto radius = later.state.position().si_value().norm(2.0, -1, false);
    out.writeln(fraction, " of a period: distance (km) = ", flat(radius.div(km)),
                ", energy (MJ/kg) = ", flat(astro::energy(later.state, mu).si_value().div(1e6)));
  }
  const auto round = astro::advance(start, common::ScalarQuantity<common::Time>::from_si(period), mu);
  out.writeln("distance from the start after one period (m) = ", flat(round.state.position().si_value().sub(start.position().si_value()).norm(2.0, -1, false)));
}
```

Output

```text
semi-major axis (km)  = [7037.9540]
eccentricity          = [0.0054]
inclination (deg)     = [7.5946]
period (min)          = [97.9331]
closest approach (km) = [7000.0000]
farthest point (km)   = [7075.9081]
energy (MJ/kg)        = [-28.3179]
0.2500 of a period: distance (km) = [7038.1587], energy (MJ/kg) = [-28.3179]
0.5000 of a period: distance (km) = [7075.9081], energy (MJ/kg) = [-28.3179]
1.0000 of a period: distance (km) = [7000.0000], energy (MJ/kg) = [-28.3179]
distance from the start after one period (m) = [0.0000]
```

The orbit is nearly circular: eccentricity `0.0054`, with the distance between `7000` km at the start (the closest point) and `7075.9081` km half a period later. The period is `97.9331` minutes, the inclination `7.59` degrees, and the energy is `-28.3179` MJ/kg at every instant, as it must be. After a whole period the satellite is back within `0.0000` m of where it began.

<!-- addex:start -->
## From a state vector to the orbit

A tracking station reports a position and velocity, and the orbit is wanted. `astro::to_elem(state, mu)` converts to the classical elements, and `astro::from_elem` converts back; the two are exact inverses. The program takes a state vector from a standard textbook example of a highly eccentric orbit, prints all six elements and the closest and farthest distances, and rebuilds the position from the elements.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double km = 1000.0, deg = std::numbers::pi / 180.0;
  const auto mu = common::ScalarQuantity<astro::GravParam>::from_si(398600.4418 * km * km * km);
  // the state vector used by Vallado for a highly eccentric orbit: position and velocity in km and km/s
  const auto state = astro::make_state(common::SeriesQuantity<common::Length>::from_si(pluss::series::make({6524.834 * km, 6862.875 * km, 6448.296 * km})),
                                       common::SeriesQuantity<common::Speed>::from_si(pluss::series::make({4.901327 * km, 5.533756 * km, -1.976341 * km})));
  const auto e = astro::to_elem(state, mu);
  out.writeln("semi-major axis (km) = ", flat(e.axis().si_value().div(km)));
  out.writeln("eccentricity         = ", flat(e.ecc().si_value()));
  out.writeln("inclination (deg)    = ", flat(e.tilt().si_value().div(deg)));
  out.writeln("node longitude (deg) = ", flat(e.node().si_value().div(deg)));
  out.writeln("periapsis angle (deg) = ", flat(e.peri().si_value().div(deg)));
  out.writeln("true anomaly (deg)   = ", flat(e.phase().si_value().div(deg)));
  // and back: the elements rebuild the same position
  const auto back = astro::from_elem(e, mu);
  out.writeln("position error after the round trip (m) = ", flat(back.position().si_value().sub(state.position().si_value()).norm(2.0, -1, false)));
  out.writeln("closest and farthest distance (km) = ", flat(astro::peri(e).si_value().div(km)), " and ", flat(astro::apo(e).si_value().div(km)));
}
```

Output

```text
semi-major axis (km) = [36127.3376]
eccentricity         = [0.8329]
inclination (deg)    = [87.8691]
node longitude (deg) = [227.8983]
periapsis angle (deg) = [53.3849]
true anomaly (deg)   = [92.3352]
position error after the round trip (m) = [0.0000]
closest and farthest distance (km) = [6038.5617] and [66216.1135]
```

The orbit has a semi-major axis of `36127.3376` km, an eccentricity of `0.8329` and an inclination of `87.8691` degrees, so it is almost polar and very elongated: the closest distance from the centre of the Earth is `6038.5617` km and the farthest `66216.1135` km. Rebuilding the position from the elements agrees with the original to `0.0000` m at the digits printed.

## Speed along an eccentric orbit

Kepler's second law says that a satellite moves fastest where it is closest. The program launches a satellite from `6578` km at `10.2` km/s, advances it by an eighth of a period at a time with the exact solution, and prints its distance and speed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const double km = 1000.0;
  const auto mu = common::ScalarQuantity<astro::GravParam>::from_si(3.986004418e14);
  // an eccentric orbit launched from 6578 km at 10.2 km/s: distance and speed every eighth of a period. The speed is highest at the closest point (Kepler second law).
  const auto start = astro::make_state(common::SeriesQuantity<common::Length>::from_si(pluss::series::make({6578.0 * km, 0.0, 0.0})),
                                       common::SeriesQuantity<common::Speed>::from_si(pluss::series::make({0.0, 10.2 * km, 0.0})));
  const double period = flat(astro::period(astro::to_elem(start, mu), mu).si_value())[0];
  out.writeln("period (hours) = ", period / 3600.0);
  for (int k = 0; k <= 8; k += 1) {
    const auto later = astro::advance(start, common::ScalarQuantity<common::Time>::from_si(period * k / 8.0), mu).state;
    out.writeln("t = ", k, "/8 of a period: distance ", flat(later.position().si_value().norm(2.0, -1, false).div(km))[0], " km, speed ", flat(later.velocity().si_value().norm(2.0, -1, false).div(km))[0], " km/s");
  }
}
```

Output

```text
period (hours) = 9.794
t = 0/8 of a period: distance 6578.000 km, speed 10.200 km/s
t = 1/8 of a period: distance 22070.324 km, speed 4.355 km/s
t = 2/8 of a period: distance 32565.695 km, speed 2.707 km/s
t = 3/8 of a period: distance 38136.551 km, speed 1.937 km/s
t = 4/8 of a period: distance 39900.634 km, speed 1.682 km/s
t = 5/8 of a period: distance 38136.551 km, speed 1.937 km/s
t = 6/8 of a period: distance 32565.695 km, speed 2.707 km/s
t = 7/8 of a period: distance 22070.324 km, speed 4.355 km/s
t = 8/8 of a period: distance 6578.000 km, speed 10.200 km/s
```

The period is `9.794` hours. The distance grows from `6578.000` km to `39900.634` km at half a period, and the speed falls from `10.200` to `1.682` km/s; the second half of the orbit mirrors the first, with the same distances and speeds at one eighth and seven eighths, and the satellite is back at `6578.000` km and `10.200` km/s after a whole period.

## Bound and unbound orbits

The sign of the specific energy decides whether an orbit is closed. The program launches three satellites from the same point at `1.0`, `1.2` and `1.5` times the circular speed, in one batch, and prints the energy, the eccentricity and whether each orbit is bound.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double km = 1000.0;
  const auto mu = common::ScalarQuantity<astro::GravParam>::from_si(3.986004418e14);
  // the orbits of three satellites in one batch: the same position, three different speeds (circular, elliptic, escape)
  const auto r = pluss::series::make({7000.0 * km, 0.0, 0.0, 7000.0 * km, 0.0, 0.0, 7000.0 * km, 0.0, 0.0}).reshape({3, 3});
  const double circular = std::sqrt(3.986004418e14 / (7000.0 * km));
  const auto v = pluss::series::make({0.0, circular, 0.0, 0.0, 1.2 * circular, 0.0, 0.0, 1.5 * circular, 0.0}).reshape({3, 3});
  const auto state = astro::make_state(common::SeriesQuantity<common::Length>::from_si(r), common::SeriesQuantity<common::Speed>::from_si(v));
  out.writeln("speed / circular speed          = 1.0000, 1.2000, 1.5000");
  out.writeln("specific energy (MJ/kg)         = ", flat(astro::energy(state, mu).si_value().div(1e6)));
  const auto e = astro::to_elem(state, mu);
  out.writeln("eccentricity                    = ", flat(e.ecc().si_value()));
  out.writeln("bound orbit? (energy below zero) = ", flat(astro::energy(state, mu).si_value().lt(0.0).to(pluss::ScalarType::Float64)));
}
```

Output

```text
speed / circular speed          = 1.0000, 1.2000, 1.5000
specific energy (MJ/kg)         = [-28.4715, -15.9440, 7.1179]
eccentricity                    = [0.0000, 0.4400, 1.2500]
bound orbit? (energy below zero) = [1.0000, 1.0000, 0.0000]
```

The first is circular (eccentricity `0.0000`, energy `-28.4715` MJ/kg), the second an ellipse (eccentricity `0.4400`, energy `-15.9440`), and the third has positive energy `7.1179` and eccentricity `1.2500`: it is a hyperbola, and the satellite escapes. Only the first two have a period.

<!-- addex:end -->

## Decisions

- Use `to_elem` when you want the shape of an orbit (axis, eccentricity, tilt, period) and `from_elem` to go back to a position and velocity.
- Use `advance` to move a state forward by any time; it needs no integration step because the orbit is solved exactly.
- Check the sign of `energy` before you ask for a period or an apoapsis: only bound orbits (negative energy) have them.
- Build quantities with `from_si` and keep every input in metres, seconds and kilograms.

## Pitfalls

A quantity built with `from_si` is already in metres. The program describes the same orbit twice: with 7000 km and 7.5 km/s converted to metres, and with the numbers 7000 and 7.5 typed in as if the unit were kilometres.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the same orbit entered twice: once in metres and metres per second, once with the numbers 7000 and 7.5 typed as if the units were kilometres
  const auto mu = common::ScalarQuantity<astro::GravParam>::from_si(3.986004418e14);
  const auto describe = [&](const char *label, double radius, double speed) {
    const auto state = astro::make_state(common::SeriesQuantity<common::Length>::from_si(pluss::series::make({radius, 0.0, 0.0})),
                                         common::SeriesQuantity<common::Speed>::from_si(pluss::series::make({0.0, speed, 0.0})));
    const auto elements = astro::to_elem(state, mu);
    out.writeln(label, ": semi-major axis (km) = ", flat(elements.axis().si_value().div(1000.0)), ", eccentricity = ", flat(elements.ecc().si_value()), ", period (min) = ", flat(astro::period(elements, mu).si_value().div(60.0)));
  };
  describe("in metres and m/s     ", 7000.0e3, 7.5e3);
  describe("km typed as metres    ", 7000.0, 7.5);
}
```

Output

```text
in metres and m/s     : semi-major axis (km) = [6915.8433], eccentricity = [0.0122], period (min) = [95.3954]
km typed as metres    : semi-major axis (km) = [3.5000], eccentricity = [1.0000], period (min) = [0.0011]
```

The correct orbit has a semi-major axis of `6915.8433` km and a period of `95.3954` minutes. With the unconverted numbers the satellite is 7 km from the centre of the Earth, the eccentricity is `1.0000` and the period is `0.0011` minutes. The library does not know what you meant, so convert first.

A period belongs to a bound orbit. The program raises the speed at 7000 km from 1.2 to 1.5 times the circular speed, and asks for the period each time.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a period exists only for a bound orbit. At 7000 km, escape speed is sqrt(2) times the circular speed, about 1.414
  const auto mu = common::ScalarQuantity<astro::GravParam>::from_si(3.986004418e14);
  const double r0 = 7000.0e3, circular = std::sqrt(3.986004418e14 / r0);
  for (double factor : {1.2, 1.4, 1.5}) {
    const auto state = astro::make_state(common::SeriesQuantity<common::Length>::from_si(pluss::series::make({r0, 0.0, 0.0})),
                                         common::SeriesQuantity<common::Speed>::from_si(pluss::series::make({0.0, factor * circular, 0.0})));
    const auto elements = astro::to_elem(state, mu);
    out.writeln("speed ", factor, " x circular: energy (MJ/kg) = ", flat(astro::energy(state, mu).si_value().div(1e6)), ", eccentricity = ", flat(elements.ecc().si_value()));
    try {
      out.writeln("    period (min) = ", flat(astro::period(elements, mu).si_value().div(60.0)));
    } catch (const pluss::Error &e) {
      out.writeln("    ", e.message());
    }
  }
}
```

Output

```text
speed 1.2000 x circular: energy (MJ/kg) = [-15.9440], eccentricity = [0.4400]
    period (min) = [231.8060]
speed 1.4000 x circular: energy (MJ/kg) = [-1.1389], eccentricity = [0.9600]
    period (min) = [12142.7430]
speed 1.5000 x circular: energy (MJ/kg) = [7.1179], eccentricity = [1.2500]
    natsci::astro::period: every orbit must be elliptic
```

At 1.4 times the circular speed the orbit is still bound, with an eccentricity of `0.9600` and a period of `12142.7430` minutes, about 8.4 days. At 1.5 times the energy is positive, the eccentricity is `1.2500`, and `period` raises the error shown rather than returning a number.

## Where next

[Symplectic integration](../phys_05_mechanics/symplectic_integration.md) advances motion that has no closed-form solution. [Heat and work](../phys_04_thermodynamics/heat_and_work.md) turns from motion to energy flow in a gas.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
