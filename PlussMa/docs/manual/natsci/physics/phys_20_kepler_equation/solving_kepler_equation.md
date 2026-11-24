# Solving Kepler's equation

[Back to Physics 20: Kepler's Equation](../phys_20_kepler_equation.md)

Where is a body on its orbit one hour from now? Time does not give the angle directly: a body on an eccentric orbit moves fast near the closest point and slowly near the farthest, and the connection between the time that has passed and the angle swept is Kepler's equation. This page converts between the three angles that describe a position on an orbit, solves the equation for one orbit or many at once, and advances orbital elements through time.

## How it works

Three angles describe the same point on an orbit with eccentricity {math}`e`: the true anomaly {math}`\nu` (the angle seen from the central body), the eccentric anomaly {math}`E` (a helper angle on the circle that encloses the ellipse), and the mean anomaly {math}`M`, which grows in proportion to time, {math}`M = M_0 + n\,t` with the mean rate {math}`n = \sqrt{\mu/a^3}`. They are tied together by

```{math}
\tan\frac{\nu}{2} = \sqrt{\frac{1+e}{1-e}}\,\tan\frac{E}{2}, \qquad M = E - e\sin E
```

and, for a hyperbola ({math}`e > 1`), by {math}`M = e\sinh H - H`. Getting {math}`M` from {math}`\nu` is direct, and `astro::to_mean(ecc, nu)` does it. Getting {math}`\nu` from {math}`M` needs the second equation solved for {math}`E`, because it cannot be inverted in closed form: `astro::solve_mean(ecc, mean)` does this by Newton's method for every element of the arrays at once, and `astro::to_true(ecc, anomaly)` converts the answer to the true anomaly. `astro::mean_rate(elements, mu)` is {math}`n`, and `astro::advance(elements, elapsed, mu)` combines all of this: it moves the mean anomaly forward by {math}`n\,t` and returns the elements with the new true anomaly. The eccentricity and the angle arrays must have the same shape, and an eccentricity of exactly {math}`1` is refused because a parabola needs a different variable. Angles are in radians.

## From the true anomaly to the mean anomaly

The program takes an orbit with eccentricity `0.5` and five points along it, from the closest point to the farthest, and prints the mean anomaly of each. It repeats the conversion for a nearly circular orbit, then solves Kepler's equation to come back to the true anomaly.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double deg = std::numbers::pi / 180.0;
  // an orbit with eccentricity 0.5: five true anomalies, from perigee to apogee
  const auto nu = vec({0.0, 45.0 * deg, 90.0 * deg, 135.0 * deg, 180.0 * deg});
  const auto ecc = vec({0.5, 0.5, 0.5, 0.5, 0.5});
  const auto mean = astro::to_mean(ecc, nu);
  out.writeln("true anomaly (deg) = ", flat(nu.div(deg)));
  out.writeln("mean anomaly (deg) = ", flat(mean.div(deg)));
  // the same angles for a nearly circular orbit: the two anomalies hardly differ
  const auto round = astro::to_mean(vec({0.01, 0.01, 0.01, 0.01, 0.01}), nu);
  out.writeln("mean anomaly, e = 0.01 (deg) = ", flat(round.div(deg)));
  // and back: solve Kepler's equation for the eccentric anomaly, then convert to the true anomaly
  const auto solved = astro::solve_mean(ecc, mean);
  out.writeln("eccentric anomaly (deg) = ", flat(solved.anomaly.div(deg)));
  out.writeln("true anomaly again (deg) = ", flat(astro::to_true(ecc, solved.anomaly).div(deg)));
  out.writeln("Newton steps = ", solved.steps, ", all converged = ", solved.converged.all().item<bool>() ? "yes" : "no");
}
```

Output

```text
true anomaly (deg) = [0.0000, 45.0000, 90.0000, 135.0000, 180.0000]
mean anomaly (deg) = [0.0000, 13.9381, 35.1902, 81.5479, 180.0000]
mean anomaly, e = 0.01 (deg) = [0.0000, 44.1940, 88.8541, 134.1854, 180.0000]
eccentric anomaly (deg) = [0.0000, 26.8990, 60.0000, 108.6857, 180.0000]
true anomaly again (deg) = [0.0000, 45.0000, 90.0000, 135.0000, 180.0000]
Newton steps = 8, all converged = yes
```

On the eccentric orbit the mean anomaly lags far behind the true anomaly: at a true anomaly of `90` degrees the mean anomaly is only `35.1902`, because the body covers the first half of its angle in a small part of the period. On the nearly circular orbit ({math}`e = 0.01`) the two angles differ by about a degree, `88.8541` against `90`. Solving Kepler's equation from the mean anomalies gives the eccentric anomalies `26.8990`, `60.0000`, `108.6857`, and `to_true` returns the original `45`, `90` and `135` degrees. The solver took `8` Newton steps and every element converged.

## Hard orbits

Newton's method for Kepler's equation is easy for a circle and harder as the orbit flattens. The program solves for a mean anomaly of `0.1` radians on five orbits, from a circle to {math}`e = 0.999`, prints the residual of the equation, and counts the steps with the default tolerance and with a looser one. It ends by adding one full turn to the mean anomaly.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // the hard case for Kepler's equation: a mean anomaly of 0.1 rad on orbits of growing eccentricity
  const auto ecc = vec({0.0, 0.5, 0.9, 0.99, 0.999});
  const auto mean = vec({0.1, 0.1, 0.1, 0.1, 0.1});
  const auto solved = astro::solve_mean(ecc, mean);
  out.writeln("eccentricity       = ", flat(ecc));
  out.writeln("eccentric anomaly  = ", flat(solved.anomaly));
  out.writeln("residual           = ", flat(solved.residual));
  out.writeln("converged          = ", flat(solved.converged.to(pluss::ScalarType::Float64)));
  // a loose tolerance and a small step budget make the solver stop earlier
  astro::KeplerOptions loose;
  loose.tolerance = 1e-4;
  loose.max_steps = 8;
  const auto quick = astro::solve_mean(ecc, mean, loose);
  out.writeln("steps with tolerance 1e-13 / 1e-4 = ", solved.steps, " / ", quick.steps);
  // a mean anomaly one full turn further gives an eccentric anomaly one full turn further
  const auto turned = astro::solve_mean(vec({0.5}), vec({0.1 + 2.0 * std::numbers::pi}));
  out.writeln("eccentric anomaly after one extra turn (e = 0.5) = ", flat(turned.anomaly), " (compare ", flat(astro::solve_mean(vec({0.5}), vec({0.1})).anomaly), ")");
}
```

Output

```text
eccentricity       = [0.000000, 0.500000, 0.900000, 0.990000, 0.999000]
eccentric anomaly  = [0.100000, 0.198695, 0.630844, 0.831660, 0.851551]
residual           = [0.000000, 0.000000, -0.000000, -0.000000, -0.000000]
converged          = [1.000000, 1.000000, 1.000000, 1.000000, 1.000000]
steps with tolerance 1e-13 / 1e-4 = 8 / 4
eccentric anomaly after one extra turn (e = 0.5) = [6.481880] (compare [0.198695])
```

The eccentric anomaly is the mean anomaly itself for the circle (`0.100000`) and shrinks toward `0.85` radians as the orbit becomes a line, where the body lingers near the closest point. All five residuals print as zero and all five elements are flagged converged. The default tolerance needed `8` steps for the whole array and the looser `1e-4` needed `4`. A mean anomaly one turn larger gives an eccentric anomaly exactly one turn larger, `6.481880` against `0.198695` plus {math}`2\pi`: the number of whole revolutions is kept.

## Advancing an orbit through time

The program builds an orbit with a `26000` km axis and eccentricity `0.7` that starts at its closest point, prints its period, checks that the mean rate times the period is one turn, and advances the elements to six fractions of a period.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const double km = 1000.0, deg = std::numbers::pi / 180.0;
  const auto mu = common::ScalarQuantity<astro::GravParam>::from_si(3.986004418e14);
  // an orbit with a 26000 km axis and eccentricity 0.7, starting at perigee (true anomaly 0)
  const auto start = astro::make_elem(common::SeriesQuantity<common::Length>::from_si(vec({26000.0 * km})),
                                      common::SeriesQuantity<common::Dimensionless>::from_si(vec({0.7})),
                                      common::SeriesQuantity<common::Dimensionless>::from_si(vec({28.0 * deg})),
                                      common::SeriesQuantity<common::Dimensionless>::from_si(vec({0.0})),
                                      common::SeriesQuantity<common::Dimensionless>::from_si(vec({0.0})),
                                      common::SeriesQuantity<common::Dimensionless>::from_si(vec({0.0})));
  const double period = flat(astro::period(start, mu).si_value())[0];
  const double rate = flat(astro::mean_rate(start, mu).si_value())[0];
  out.writeln("period (h) = ", period / 3600.0, ", mean rate x period / (2 pi) = ", rate * period / (2.0 * std::numbers::pi));
  // advance the elements: the true anomaly changes quickly near perigee and slowly near apogee
  for (double fraction : {0.05, 0.1, 0.25, 0.5, 0.75, 0.95}) {
    const auto later = astro::advance(start, common::ScalarQuantity<common::Time>::from_si(period * fraction), mu);
    out.writeln(fraction, " of a period: true anomaly (deg) = ", flat(later.elements.phase().si_value().div(deg)), ", eccentric anomaly (deg) = ", flat(later.solve.anomaly.div(deg)));
  }
}
```

Output

```text
period (h) = 11.5896, mean rate x period / (2 pi) = 1.0000
0.0500 of a period: true anomaly (deg) = [92.8339], eccentric anomaly (deg) = [47.6327]
0.1000 of a period: true anomaly (deg) = [122.3240], eccentric anomaly (deg) = [74.6823]
0.2500 of a period: true anomaly (deg) = [154.5402], eccentric anomaly (deg) = [123.4601]
0.5000 of a period: true anomaly (deg) = [180.0000], eccentric anomaly (deg) = [180.0000]
0.7500 of a period: true anomaly (deg) = [205.4598], eccentric anomaly (deg) = [236.5399]
0.9500 of a period: true anomaly (deg) = [267.1661], eccentric anomaly (deg) = [312.3673]
```

The period is `11.5896` hours, and `mean_rate x period / (2 pi)` is `1.0000`, one turn. The motion is fast at first and slow later: after only `5` per cent of the period the true anomaly has already reached `92.8339` degrees, after `25` per cent `154.5402`, and half a period brings it to `180.0000`, the farthest point. The second half mirrors the first: `205.4598` at three quarters, as `154.5402` is the mirror image of it about `180`.

## Decisions

- Use `advance(elements, elapsed, mu)` when you want the position some time later; use `to_mean`, `solve_mean` and `to_true` yourself when you need the angles in between, for example the mean anomaly of an observed point or the eccentric anomaly to compute the distance.
- Pass whole arrays of eccentricities and angles to one `solve_mean` call: the solver iterates all of them together, and the step count it reports is the count for the slowest element.
- Leave `KeplerOptions` alone unless you need a different accuracy. A tolerance of `0` picks a default for the precision of the data, and `strict = true` makes a solve that did not converge an error. Set `strict = false` and read `converged` when you want to examine the failure yourself.
- For an escape trajectory use the hyperbolic branch: give {math}`e > 1` and a negative semi-major axis. `to_mean`, `solve_mean` and `to_true` handle it, but `period` and `advance` on elements are for elliptic orbits only.

## Pitfalls

The program tries the inputs that do not fit: a period of a hyperbola, a parabola, a negative eccentricity, arrays of different sizes, and a solver that is given too few steps. It spells the namespace out in full to show where each function lives.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <numbers>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace astro = natsci::physics::astro;
namespace common = natsci::common;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the hyperbolic branch of Kepler's equation works, but a hyperbola has no period
  const auto h = pluss::ma::natsci::physics::astro::solve_mean(vec({1.5}), vec({3.0}));
  out.writeln("hyperbolic anomaly for e = 1.5, M = 3: ", flat(h.anomaly), ", true anomaly (deg): ", flat(pluss::ma::natsci::physics::astro::to_true(vec({1.5}), h.anomaly).mul(180.0 / std::numbers::pi)));
  const auto mu = common::ScalarQuantity<pluss::ma::natsci::physics::astro::GravParam>::from_si(3.986004418e14);
  const auto open = pluss::ma::natsci::physics::astro::make_elem(common::SeriesQuantity<common::Length>::from_si(vec({-20000e3})),
                                     common::SeriesQuantity<common::Dimensionless>::from_si(vec({1.5})),
                                     common::SeriesQuantity<common::Dimensionless>::from_si(vec({0.0})),
                                     common::SeriesQuantity<common::Dimensionless>::from_si(vec({0.0})),
                                     common::SeriesQuantity<common::Dimensionless>::from_si(vec({0.0})),
                                     common::SeriesQuantity<common::Dimensionless>::from_si(vec({0.0})));
  try { (void)pluss::ma::natsci::physics::astro::period(open, mu); } catch (const pluss::Error &e) { out.writeln("period of a hyperbola: ", e.message()); }
  try { (void)pluss::ma::natsci::physics::astro::to_mean(vec({1.0}), vec({0.5})); } catch (const pluss::Error &e) { out.writeln("e = 1: ", e.message()); }
  try { (void)pluss::ma::natsci::physics::astro::to_mean(vec({-0.1}), vec({0.5})); } catch (const pluss::Error &e) { out.writeln("e < 0: ", e.message()); }
  try { (void)pluss::ma::natsci::physics::astro::to_mean(vec({0.5, 0.5}), vec({0.5})); } catch (const pluss::Error &e) { out.writeln("shapes: ", e.message()); }
  pluss::ma::natsci::physics::astro::KeplerOptions tight;
  tight.max_steps = 1;
  try { (void)pluss::ma::natsci::physics::astro::solve_mean(vec({0.999}), vec({0.001}), tight); } catch (const pluss::Error &e) { out.writeln("one step: ", e.message()); }
  tight.strict = false;
  const auto lax = pluss::ma::natsci::physics::astro::solve_mean(vec({0.999}), vec({0.001}), tight);
  out.writeln("with strict = false: converged = ", lax.converged.all().item<bool>() ? "yes" : "no", ", residual ", flat(lax.residual));
}
```

Output

```text
hyperbolic anomaly for e = 1.5, M = 3: [1.8995], true anomaly (deg): [117.6840]
period of a hyperbola: natsci::astro::period: every orbit must be elliptic
e = 1: natsci::astro::to_mean: parabolic anomalies require a universal variable representation
e < 0: natsci::astro::to_mean: eccentricity must be non-negative
shapes: natsci::astro::to_mean: anomaly shape must match the other element fields
one step: natsci::astro::solve_mean: Kepler solve did not converge
with strict = false: converged = no, residual [0.0167]
```

The hyperbolic branch of Kepler's equation works (`H = 1.8995` radians for {math}`e = 1.5` and {math}`M = 3`), but asking for the period of a hyperbola, an eccentricity of `1`, a negative eccentricity and mismatched array sizes are all rejected with a message that names the function. A solver that cannot converge in the steps it is allowed stops with an error. With `strict = false` it returns instead, with `converged = no` and the residual `0.0167` that remains: always test `converged` when you turn strictness off.

## Where next

[Physics 1: Orbits](../phys_01_orbits.md) converts a position and a velocity into elements and flies the body for any time with `advance` on a state, and [Physics 21: Orbit Transfers](../phys_21_transfers.md) finds the velocity that carries a body from one point to another in a given time.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
