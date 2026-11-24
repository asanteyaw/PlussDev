# Paths and conservation drift

[Back to Methods 3: Measurement and Uncertainty](../meth_03_uncertainty.md)

A simulation returns states at a sequence of times, and you want to keep them together with their coordinate and check that an invariant such as the energy was conserved. This page packs the recorded states of a mass on a spring into a `Trajectory` that checks the ordering of the coordinate, reads the fields back by name, and measures the drift of the energy against a scale you choose, for a fine and for a coarse integration.

## How it works

`pluss::ma::natsci::common::states(field(...), ...)` packs named, dimensioned quantities, each of the shape `[samples, ...]`, into one `StateVector`: `field("position", SeriesQuantity<Length>, "x")` gives one. `common::path(coordinate, state)` pairs the states with a coordinate such as the time, and returns a `Trajectory` that checks that every state keeps the axes of the coordinate and that the coordinate is strictly increasing (pass `false` as the last argument to allow a coordinate that is not ordered, such as a path in a periodic variable). `trajectory.coordinate()`, `trajectory.state().get<D>("name")`, `trajectory.steps()` and `trajectory.ordered()` read it back, and `state().flat()` is the whole state as one array. Numerical integrators of the library return such paths.

To judge an integration you compare an invariant {math}`I` with its starting value. `common::drift(reference, current, scale)` returns the absolute drift {math}`|I - I_0|` and the scaled drift

```{math}
\text{scaled drift} = \frac{|I - I_0|}{\text{scale}}
```

The scale is a physical quantity of the same dimension that you pass: a unit-dependent small constant hidden in the formula would make the result depend on the units. The programs integrate {math}`\dot x = v`, {math}`\dot v = -\omega^2 x` with {math}`\omega = 2` and the shared fourth-order integrator, recorded every `0.25` s, and use the energy per unit mass {math}`\tfrac12 v^2 + \tfrac12\omega^2 x^2`, which is `2` at the start.

## A recorded path

The program integrates the oscillator for two time units, packs the position and the speed with the times into a path, and reads the coordinate and both fields back.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace units = common::units;
namespace dynamics = common::dynamics;
using pluss::Series;
using Energy2 = common::DimensionPower<common::Speed, 2>;   // energy per unit mass, (m/s)^2
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }

// a mass on a spring, omega = 2: x' = v, v' = -omega^2 x, integrated with the shared RK4 engine and recorded every 0.25 s
static common::Trajectory<common::Time> oscillator(std::int64_t steps_per_sample, double horizon) {
  const double omega = 2.0;
  const auto rate = [=](const Series &y) { return y.select(-1, 1).unsqueeze(-1).unite(std::vector<Series>{y.select(-1, 0).mul(-omega * omega).unsqueeze(-1)}, -1); };
  const auto samples = static_cast<std::int64_t>(horizon / 0.25);
  const auto run = dynamics::trajectory(rate, vec({1.0, 0.0}), 0.0, horizon, samples, steps_per_sample);
  const auto position = common::field("position", common::SeriesQuantity<common::Length>::from_si(run.states.select(-1, 0)), "x");
  const auto speed = common::field("speed", common::SeriesQuantity<common::Speed>::from_si(run.states.select(-1, 1)), "v");
  return common::path(common::SeriesQuantity<common::Time>::from_si(run.times), common::states(position, speed));
}
static Series energy(const common::Trajectory<common::Time> &orbit) {
  const double omega2 = 4.0;
  const auto x = orbit.state().get<common::Length>("position").si_value(), v = orbit.state().get<common::Speed>("speed").si_value();
  return v.square().mul(0.5).add(x.square().mul(0.5 * omega2));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto orbit = oscillator(50, 2.0);
  out.writeln("samples along the path: ", orbit.steps(), ", ordered: ", orbit.ordered() ? "yes" : "no");
  out.writeln("time     ", flat(orbit.coordinate().si_value()));
  out.writeln("position ", flat(orbit.state().get<common::Length>("position").si_value()));
  out.writeln("speed    ", flat(orbit.state().get<common::Speed>("speed").si_value()));
  out.writeln("fields: ", orbit.state().size(), ", flat state shape: ", orbit.state().flat().sizes());
}
```

Output

```text
samples along the path: 9, ordered: yes
time     [0.00000, 0.25000, 0.50000, 0.75000, 1.00000, 1.25000, 1.50000, 1.75000, 2.00000]
position [1.00000, 0.87758, 0.54030, 0.07074, -0.41615, -0.80114, -0.98999, -0.93646, -0.65364]
speed    [0.00000, -0.95885, -1.68294, -1.99499, -1.81859, -1.19694, -0.28224, 0.70157, 1.51360]
fields: 2, flat state shape: [18]
```

The path has `9` samples, ordered. The position starts at `1.00000` and the speed at `0.00000`, and the position crosses zero near {math}`t = 0.75` (`0.07074`) with the speed at its extreme there (`-1.99499`). The state holds `2` fields of `9` samples each, `18` numbers in all.

## The drift of an invariant

The program integrates for ten time units twice: with `50` steps of the integrator between records, and with only `2`. It reports the largest absolute and scaled drift of the energy against the scale `2`, the initial energy.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace units = common::units;
namespace dynamics = common::dynamics;
using pluss::Series;
using Energy2 = common::DimensionPower<common::Speed, 2>;   // energy per unit mass, (m/s)^2
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }

// a mass on a spring, omega = 2: x' = v, v' = -omega^2 x, integrated with the shared RK4 engine and recorded every 0.25 s
static common::Trajectory<common::Time> oscillator(std::int64_t steps_per_sample, double horizon) {
  const double omega = 2.0;
  const auto rate = [=](const Series &y) { return y.select(-1, 1).unsqueeze(-1).unite(std::vector<Series>{y.select(-1, 0).mul(-omega * omega).unsqueeze(-1)}, -1); };
  const auto samples = static_cast<std::int64_t>(horizon / 0.25);
  const auto run = dynamics::trajectory(rate, vec({1.0, 0.0}), 0.0, horizon, samples, steps_per_sample);
  const auto position = common::field("position", common::SeriesQuantity<common::Length>::from_si(run.states.select(-1, 0)), "x");
  const auto speed = common::field("speed", common::SeriesQuantity<common::Speed>::from_si(run.states.select(-1, 1)), "v");
  return common::path(common::SeriesQuantity<common::Time>::from_si(run.times), common::states(position, speed));
}
static Series energy(const common::Trajectory<common::Time> &orbit) {
  const double omega2 = 4.0;
  const auto x = orbit.state().get<common::Length>("position").si_value(), v = orbit.state().get<common::Speed>("speed").si_value();
  return v.square().mul(0.5).add(x.square().mul(0.5 * omega2));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9).with_sep(""));
  // the energy 0.5 v^2 + 0.5 omega^2 x^2 is conserved by the exact motion; compare a fine and a coarse integration of it
  const auto scale = common::ScalarQuantity<Energy2>::from_si(2.0);   // the initial energy
  for (const std::int64_t substeps : {50, 2}) {
    const auto orbit = oscillator(substeps, 10.0);
    const Series e = energy(orbit);
    const Series reference = e.mul(0.0).add(e.select(0, 0));
    const auto d = common::drift(common::SeriesQuantity<Energy2>::from_si(reference), common::SeriesQuantity<Energy2>::from_si(e), scale);
    out.writeln(substeps, " RK4 steps per 0.25 s sample:  largest absolute drift = ", d.absolute.si_value().max().item<double>(), ", largest scaled drift = ", d.scaled.si_value().max().item<double>());
  }
}
```

Output

```text
50 RK4 steps per 0.25 s sample:  largest absolute drift = 0.000000000, largest scaled drift = 0.000000000
2 RK4 steps per 0.25 s sample:  largest absolute drift = 0.000538225, largest scaled drift = 0.000269112
```

With `50` steps per record the drift is below `1e-9` and prints as `0.000000000`. With `2` steps it is `0.000538225` absolute and `0.000269112` scaled: about `0.03` percent of the initial energy, which is the price of the coarse step. The scaled drift is dimensionless, so it can be compared across systems and units.

## Where the drift builds up

The scaled drift of every record of the coarse run says when the invariant is lost. The program prints it at six of the recorded times.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace units = common::units;
namespace dynamics = common::dynamics;
using pluss::Series;
using Energy2 = common::DimensionPower<common::Speed, 2>;   // energy per unit mass, (m/s)^2
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }

// a mass on a spring, omega = 2: x' = v, v' = -omega^2 x, integrated with the shared RK4 engine and recorded every 0.25 s
static common::Trajectory<common::Time> oscillator(std::int64_t steps_per_sample, double horizon) {
  const double omega = 2.0;
  const auto rate = [=](const Series &y) { return y.select(-1, 1).unsqueeze(-1).unite(std::vector<Series>{y.select(-1, 0).mul(-omega * omega).unsqueeze(-1)}, -1); };
  const auto samples = static_cast<std::int64_t>(horizon / 0.25);
  const auto run = dynamics::trajectory(rate, vec({1.0, 0.0}), 0.0, horizon, samples, steps_per_sample);
  const auto position = common::field("position", common::SeriesQuantity<common::Length>::from_si(run.states.select(-1, 0)), "x");
  const auto speed = common::field("speed", common::SeriesQuantity<common::Speed>::from_si(run.states.select(-1, 1)), "v");
  return common::path(common::SeriesQuantity<common::Time>::from_si(run.times), common::states(position, speed));
}
static Series energy(const common::Trajectory<common::Time> &orbit) {
  const double omega2 = 4.0;
  const auto x = orbit.state().get<common::Length>("position").si_value(), v = orbit.state().get<common::Speed>("speed").si_value();
  return v.square().mul(0.5).add(x.square().mul(0.5 * omega2));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // a drift history: the scaled drift at every recorded time says WHEN a run starts to lose the invariant
  const auto orbit = oscillator(2, 10.0);
  const Series e = energy(orbit);
  const auto d = common::drift(common::SeriesQuantity<Energy2>::from_si(e.mul(0.0).add(e.select(0, 0))), common::SeriesQuantity<Energy2>::from_si(e), common::ScalarQuantity<Energy2>::from_si(2.0));
  const auto scaled = flat(d.scaled.si_value());
  for (const std::size_t i : {std::size_t{0}, std::size_t{8}, std::size_t{16}, std::size_t{24}, std::size_t{32}, std::size_t{40}})
    out.writeln("t = ", flat(orbit.coordinate().si_value())[i], "   scaled drift = ", scaled[i]);
}
```

Output

```text
t = 0.000000   scaled drift = 0.000000
t = 2.000000   scaled drift = 0.000054
t = 4.000000   scaled drift = 0.000108
t = 6.000000   scaled drift = 0.000161
t = 8.000000   scaled drift = 0.000215
t = 10.000000   scaled drift = 0.000269
```

The scaled drift grows steadily, `0.000054` at {math}`t = 2`, `0.000161` at `6` and `0.000269` at `10`: a numerical damping of the oscillation, `0.000027` per time unit. A drift that grows in proportion to time is a systematic error of the method; a drift that oscillates around zero would point to a noisy step size.

## Decisions

- Pass the scale that is natural for the invariant, normally its initial value, as a `ScalarQuantity` of the invariant's dimension. A scale of zero or a non-finite scale is refused.
- Compare drift histories at the same scale across methods and steps: the scaled drift is a pure number.
- Keep `require_ordered` on for time-like coordinates. Turn it off only for a coordinate that is not monotone, such as an angle that wraps.
- Collect the states into one `StateVector` first: it checks that every field has the axes of the coordinate, so a field that was sampled at other times is found at once.

## Pitfalls

A coordinate that runs backwards, repeats or has a different number of samples than the states is not a path. The program tries all three.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace units = common::units;
namespace dynamics = common::dynamics;
using pluss::Series;
using Energy2 = common::DimensionPower<common::Speed, 2>;   // energy per unit mass, (m/s)^2
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }

// a mass on a spring, omega = 2: x' = v, v' = -omega^2 x, integrated with the shared RK4 engine and recorded every 0.25 s
static common::Trajectory<common::Time> oscillator(std::int64_t steps_per_sample, double horizon) {
  const double omega = 2.0;
  const auto rate = [=](const Series &y) { return y.select(-1, 1).unsqueeze(-1).unite(std::vector<Series>{y.select(-1, 0).mul(-omega * omega).unsqueeze(-1)}, -1); };
  const auto samples = static_cast<std::int64_t>(horizon / 0.25);
  const auto run = dynamics::trajectory(rate, vec({1.0, 0.0}), 0.0, horizon, samples, steps_per_sample);
  const auto position = common::field("position", common::SeriesQuantity<common::Length>::from_si(run.states.select(-1, 0)), "x");
  const auto speed = common::field("speed", common::SeriesQuantity<common::Speed>::from_si(run.states.select(-1, 1)), "v");
  return common::path(common::SeriesQuantity<common::Time>::from_si(run.times), common::states(position, speed));
}
static Series energy(const common::Trajectory<common::Time> &orbit) {
  const double omega2 = 4.0;
  const auto x = orbit.state().get<common::Length>("position").si_value(), v = orbit.state().get<common::Speed>("speed").si_value();
  return v.square().mul(0.5).add(x.square().mul(0.5 * omega2));
}

int main() {
  OutputEngine out;
  // a time coordinate that runs backwards, or repeats, is not a path
  const auto x = common::field("position", common::SeriesQuantity<common::Length>::from_si(vec({1.0, 0.9, 0.8})), "x");
  try {
    (void)common::path(common::SeriesQuantity<common::Time>::from_si(vec({0.0, 2.0, 1.0})), common::states(x));
  } catch (const pluss::Error &e) {
    out.writeln("out-of-order time:  ", e.message());
  }
  try {
    (void)common::path(common::SeriesQuantity<common::Time>::from_si(vec({0.0, 1.0, 1.0})), common::states(x));
  } catch (const pluss::Error &e) {
    out.writeln("repeated time:      ", e.message());
  }
  try {
    (void)common::path(common::SeriesQuantity<common::Time>::from_si(vec({0.0, 1.0})), common::states(x));
  } catch (const pluss::Error &e) {
    out.writeln("two times, three states: ", e.message());
  }
}
```

Output

```text
out-of-order time:   natsci::common::Trajectory: coordinate must be strictly increasing
repeated time:       natsci::common::Trajectory: coordinate must be strictly increasing
two times, three states:  natsci::common::Trajectory: state shape does not begin with coordinate shape
```

The first two are refused with `coordinate must be strictly increasing`: out-of-order and repeated times are equally wrong, since many recorded steps do not move forward. The third is refused because the state does not begin with the shape of the coordinate.

The scaled drift is only as meaningful as the scale. The program computes it with the initial energy as the scale and with a scale a thousand times smaller.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace units = common::units;
namespace dynamics = common::dynamics;
using pluss::Series;
using Energy2 = common::DimensionPower<common::Speed, 2>;   // energy per unit mass, (m/s)^2
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }

// a mass on a spring, omega = 2: x' = v, v' = -omega^2 x, integrated with the shared RK4 engine and recorded every 0.25 s
static common::Trajectory<common::Time> oscillator(std::int64_t steps_per_sample, double horizon) {
  const double omega = 2.0;
  const auto rate = [=](const Series &y) { return y.select(-1, 1).unsqueeze(-1).unite(std::vector<Series>{y.select(-1, 0).mul(-omega * omega).unsqueeze(-1)}, -1); };
  const auto samples = static_cast<std::int64_t>(horizon / 0.25);
  const auto run = dynamics::trajectory(rate, vec({1.0, 0.0}), 0.0, horizon, samples, steps_per_sample);
  const auto position = common::field("position", common::SeriesQuantity<common::Length>::from_si(run.states.select(-1, 0)), "x");
  const auto speed = common::field("speed", common::SeriesQuantity<common::Speed>::from_si(run.states.select(-1, 1)), "v");
  return common::path(common::SeriesQuantity<common::Time>::from_si(run.times), common::states(position, speed));
}
static Series energy(const common::Trajectory<common::Time> &orbit) {
  const double omega2 = 4.0;
  const auto x = orbit.state().get<common::Length>("position").si_value(), v = orbit.state().get<common::Speed>("speed").si_value();
  return v.square().mul(0.5).add(x.square().mul(0.5 * omega2));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // drift is measured against a scale YOU choose: the scale must be the size of the invariant, not a unit-dependent epsilon
  const auto orbit = oscillator(2, 10.0);
  const Series e = energy(orbit);
  const auto ref = common::SeriesQuantity<Energy2>::from_si(e.mul(0.0).add(e.select(0, 0)));
  const auto cur = common::SeriesQuantity<Energy2>::from_si(e);
  for (const double s : {2.0, 0.002}) {
    const auto d = common::drift(ref, cur, common::ScalarQuantity<Energy2>::from_si(s));
    out.writeln("scale = ", s, ":  largest scaled drift = ", d.scaled.si_value().max().item<double>());
  }
  try {
    (void)common::drift(ref, cur, common::ScalarQuantity<Energy2>::from_si(0.0));
  } catch (const pluss::Error &e) {
    out.writeln("zero scale: ", e.message());
  }
}
```

Output

```text
scale = 2.000000:  largest scaled drift = 0.000269
scale = 0.002000:  largest scaled drift = 0.269112
zero scale: natsci::common::drift: scale must be finite and positive
```

With the scale `2.000000` the largest scaled drift is `0.000269`; with `0.002000` it is `0.269112`, the same run reported as if it had lost a quarter of its energy. A zero scale is refused. Choose the scale by the size of the invariant, not by the size of the tolerance you hope to meet.

Reference and current values must have the same shape, dtype and platform.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace units = common::units;
namespace dynamics = common::dynamics;
using pluss::Series;
using Energy2 = common::DimensionPower<common::Speed, 2>;   // energy per unit mass, (m/s)^2
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }
static Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }

// a mass on a spring, omega = 2: x' = v, v' = -omega^2 x, integrated with the shared RK4 engine and recorded every 0.25 s
static common::Trajectory<common::Time> oscillator(std::int64_t steps_per_sample, double horizon) {
  const double omega = 2.0;
  const auto rate = [=](const Series &y) { return y.select(-1, 1).unsqueeze(-1).unite(std::vector<Series>{y.select(-1, 0).mul(-omega * omega).unsqueeze(-1)}, -1); };
  const auto samples = static_cast<std::int64_t>(horizon / 0.25);
  const auto run = dynamics::trajectory(rate, vec({1.0, 0.0}), 0.0, horizon, samples, steps_per_sample);
  const auto position = common::field("position", common::SeriesQuantity<common::Length>::from_si(run.states.select(-1, 0)), "x");
  const auto speed = common::field("speed", common::SeriesQuantity<common::Speed>::from_si(run.states.select(-1, 1)), "v");
  return common::path(common::SeriesQuantity<common::Time>::from_si(run.times), common::states(position, speed));
}
static Series energy(const common::Trajectory<common::Time> &orbit) {
  const double omega2 = 4.0;
  const auto x = orbit.state().get<common::Length>("position").si_value(), v = orbit.state().get<common::Speed>("speed").si_value();
  return v.square().mul(0.5).add(x.square().mul(0.5 * omega2));
}

int main() {
  OutputEngine out;
  const auto a = common::SeriesQuantity<Energy2>::from_si(vec({1.0, 1.0, 1.0}));
  const auto b = common::SeriesQuantity<Energy2>::from_si(vec({1.0, 1.1}));
  try {
    (void)common::drift(a, b, common::ScalarQuantity<Energy2>::from_si(1.0));
  } catch (const pluss::Error &e) {
    out.writeln("different lengths: ", e.message());
  }
  const auto f32 = common::SeriesQuantity<Energy2>::from_si(vec({1.0, 1.1, 1.2}).to(pluss::ScalarType::Float32));
  try {
    (void)common::drift(a, f32, common::ScalarQuantity<Energy2>::from_si(1.0));
  } catch (const pluss::Error &e) {
    out.writeln("Float64 against Float32: ", e.message());
  }
}
```

Output

```text
different lengths:  natsci::common::drift: shape mismatch
Float64 against Float32:  natsci::common::drift: dtype mismatch
```

A reference of three samples against a current history of two is refused, and so is a Float64 reference against Float32 values: the comparison is exact, and a silent conversion would hide a loss of precision in the very thing you are measuring.

## Where next

[Measurements with an uncertainty](measurements.md) and [Correlated measurements](correlated_measurements.md) carry uncertainties through the quantities you read from a path. The integrators that produce paths are used in [Physics 5: Mechanics](../../physics/phys_05_mechanics.md) and [Physics 1: Orbits](../../physics/phys_01_orbits.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
