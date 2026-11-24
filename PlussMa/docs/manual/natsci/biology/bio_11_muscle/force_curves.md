# Force curves

[Back to Biology 11: Muscle](../bio_11_muscle.md)

A muscle's force depends on how long its fibers are, how fast they are changing length, and how stretched its tendon is. This page evaluates the curves of a Hill-type muscle model that describe each dependence, in normalised units that apply to any muscle.

## How it works

Lengths are measured in units of the optimal fiber length and forces in units of the maximum isometric force, so the same curves describe every muscle. `active_force_length(l)` is the force the contractile fibers produce at full activation: it peaks at `1` at the optimal length and falls on both sides. `passive_force_length(l)` is the force of the stretched, inactive tissue, which is negligible until the fiber is lengthened well past the optimum. `force_velocity(v)` multiplies the active force by a factor that is `1` at zero velocity, below `1` while the fiber shortens (negative `v`) and above `1` while it lengthens. `tendon_force_length(l)` gives the force of a tendon of length `l` in units of its slack length: zero at slack and about `1` at its normal maximum strain of five percent. Every function takes an array of any shape.

## Four curves

The program evaluates the active and passive force-length curves at five fiber lengths, the force-velocity factor at five velocities and the tendon force at four tendon lengths.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace muscle = biology::biomechanics::muscle;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // all lengths are in units of the optimal fiber length, all forces in units of the maximum isometric force
  const auto length = vec({0.6, 0.8, 1.0, 1.2, 1.4});
  out.writeln("fiber length            ", flat(length));
  out.writeln("active force-length     ", flat(muscle::active_force_length(length)));
  out.writeln("passive force-length    ", flat(muscle::passive_force_length(length)));
  const auto velocity = vec({-1.0, -0.5, 0.0, 0.5, 1.0});
  out.writeln("fiber velocity          ", flat(velocity));
  out.writeln("force-velocity          ", flat(muscle::force_velocity(velocity)));
  const auto tendon = vec({1.00, 1.02, 1.04, 1.05});
  out.writeln("tendon length           ", flat(tendon));
  out.writeln("tendon force            ", flat(muscle::tendon_force_length(tendon)));
}
```

Output

```text
fiber length            [0.6000, 0.8000, 1.0000, 1.2000, 1.4000]
active force-length     [0.3021, 0.8364, 1.0000, 0.7937, 0.3802]
passive force-length    [0.0012, 0.0047, 0.0182, 0.0694, 0.2635]
fiber velocity          [-1.0000, -0.5000, 0.0000, 0.5000, 1.0000]
force-velocity          [0.0000, 0.2340, 1.0000, 1.5884, 1.7943]
tendon length           [1.0000, 1.0200, 1.0400, 1.0500]
tendon force            [0.0000, 0.2156, 0.6635, 1.0447]
```

The active force is `1.0000` at the optimal length and drops to `0.3021` at `0.6` and `0.3802` at `1.4`, while the passive force is `0.0182` at the optimum and rises to `0.2635` at `1.4`. The force-velocity factor is `0.0000` at the maximum shortening speed, `1.0000` when the fiber is still and `1.7943` at the fastest lengthening. The tendon is slack at `1.00`, carries `0.2156` of the maximum force at `1.02` and `1.0447` at a strain of `5%`.

## Inverting the curves

The curves are used backwards too. `force_velocity_inverse(multiplier)` gives the fiber velocity that produces a force multiplier, and `tendon_length_from_force(force)` the tendon length that carries a force. The program checks the round trip for both.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace muscle = biology::biomechanics::muscle;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the curves can be inverted: the fiber velocity that produces a given force multiplier, and the tendon length that carries a given force
  const auto velocity = vec({-0.8, -0.4, 0.0, 0.4, 0.8});
  const auto multiplier = muscle::force_velocity(velocity);
  out.writeln("velocity                 ", flat(velocity));
  out.writeln("force multiplier         ", flat(multiplier));
  out.writeln("recovered velocity       ", flat(muscle::force_velocity_inverse(multiplier)));
  const auto force = vec({0.0, 0.25, 0.5, 1.0});
  const auto length = muscle::tendon_length_from_force(force);
  out.writeln("tendon force             ", flat(force));
  out.writeln("tendon length            ", flat(length));
  out.writeln("force from that length   ", flat(muscle::tendon_force_length(length)));
}
```

Output

```text
velocity                 [-0.8000, -0.4000, 0.0000, 0.4000, 0.8000]
force multiplier         [0.0748, 0.3104, 1.0000, 1.5254, 1.7268]
recovered velocity       [-0.8000, -0.4000, -0.0000, 0.4000, 0.8000]
tendon force             [0.0000, 0.2500, 0.5000, 1.0000]
tendon length            [1.0000, 1.0222, 1.0343, 1.0490]
force from that length   [0.0000, 0.2500, 0.5000, 1.0000]
```

A force multiplier of `0.0748` to `1.7268` corresponds to velocities from `-0.8` to `0.8`, and the inverse returns the original velocities to four digits. A tendon carrying `0.25` of the maximum force is only `2.2%` longer than its slack length (`1.0222`), and the force from that length is exactly the input.

## Different muscles, different curves

The shapes differ between muscles. `active_force_length(l, width_scale)` widens or narrows the active curve, and `passive_force_length(l, strain_at_one)` sets how stiff the passive tissue is, by the strain at which it carries the maximum isometric force.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace muscle = biology::biomechanics::muscle;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a narrower or wider active force-length curve (width_scale), and a stiffer or softer passive curve (strain at unit force)
  const auto length = vec({0.6, 0.8, 1.0, 1.2, 1.4});
  out.writeln("fiber length               ", flat(length));
  for (double w : {0.7, 1.0, 1.4}) out.writeln("active, width scale ", w, "      ", flat(muscle::active_force_length(length, w)));
  for (double e : {0.4, 0.6, 0.9}) out.writeln("passive, strain at unit ", e, "  ", flat(muscle::passive_force_length(length, e)));
}
```

Output

```text
fiber length               [0.6000, 0.8000, 1.0000, 1.2000, 1.4000]
active, width scale 0.7000      [0.0305, 0.7170, 1.0000, 0.6121, 0.1485]
active, width scale 1.0000      [0.3021, 0.8364, 1.0000, 0.7937, 0.3802]
active, width scale 1.4000      [0.7170, 0.8945, 1.0000, 0.8959, 0.6121]
passive, strain at unit 0.4000  [0.0003, 0.0025, 0.0183, 0.1353, 1.0000]
passive, strain at unit 0.6000  [0.0012, 0.0047, 0.0182, 0.0694, 0.2635]
passive, strain at unit 0.9000  [0.0026, 0.0070, 0.0178, 0.0441, 0.1079]
```

A narrow active curve (`0.7`) gives `0.0305` at fiber length `0.6` and `0.1485` at `1.4`, a wide one (`1.4`) gives `0.7170` and `0.6121`. A stiff passive curve, with unit force at a strain of `0.4`, reaches `1.0000` at length `1.4`, while a compliant one, strain `0.9`, only `0.1079`.

## Activation dynamics

Force is not produced the instant a neural excitation arrives. `activation_rate(activation, excitation, params)` is the rate of change of activation, which rises fast and falls more slowly. The program prints the rates for full and for zero excitation at three activation levels, and then steps a pulse of excitation with `1` ms Euler steps.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace muscle = biology::biomechanics::muscle;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // activation follows the neural excitation with a different speed up and down: the rate at activation 0.2, 0.5, 0.8 for full excitation and for none
  muscle::Params p;
  const auto activation = vec({0.2, 0.5, 0.8});
  out.writeln("activation                    ", flat(activation));
  out.writeln("rate with full excitation     ", flat(muscle::activation_rate(activation, vec({1.0, 1.0, 1.0}), p)));
  out.writeln("rate with no excitation       ", flat(muscle::activation_rate(activation, vec({0.0, 0.0, 0.0}), p)));
  // an activation pulse followed through time by explicit Euler steps of 1 ms
  double a = 0.0;
  for (int ms = 1; ms <= 200; ++ms) {
    const double e = ms <= 100 ? 1.0 : 0.0;
    a += 0.001 * flat(muscle::activation_rate(vec({a}), vec({e}), p))[0];
    if (ms == 20 || ms == 50 || ms == 100 || ms == 150 || ms == 200) out.writeln("t = ", ms, " ms: excitation ", e, ", activation ", a);
  }
}
```

Output

```text
activation                    [0.2000, 0.5000, 0.8000]
rate with full excitation     [40.9019, 18.9476, 6.7767]
rate with no excitation       [-9.5267, -18.1358, -26.6721]
t = 20 ms: excitation 1.0000, activation 0.6009
t = 50 ms: excitation 1.0000, activation 0.8602
t = 100 ms: excitation 1.0000, activation 0.9744
t = 150 ms: excitation 0.0000, activation 0.1262
t = 200 ms: excitation 0.0000, activation 0.0045
```

With full excitation activation rises at `40.9019` per second from `0.2` and more slowly as it approaches `1`; with none it falls at `-9.5267` to `-26.6721` per second. In the pulse, activation reaches `0.6009` after `20` ms and `0.9744` after `100` ms, and after the excitation stops it falls to `0.1262` in `50` ms and to `0.0045` after `100`.

## Decisions

- Work in normalised units everywhere: divide lengths by the optimal fiber length and forces by the maximum isometric force, and rescale only when you report a result.
- Use the inverse functions (`force_velocity_inverse`, `tendon_length_from_force`) when the force is known and the length or velocity is wanted, instead of solving by hand.
- Change `width_scale` and `strain_at_one` to describe a particular muscle; leave them at their defaults for a generic one.

## Pitfalls

The curves expect normalised lengths and speeds. The program passes a fiber length in metres by mistake, and then speeds beyond the maximum shortening speed.

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

namespace muscle = biology::biomechanics::muscle;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // lengths are in units of the optimal fiber length: 0.12 m of fiber is 1.2, not 0.12
  out.writeln("active force at 0.12 (metres by mistake) = ", flat(muscle::active_force_length(vec({0.12}))), ", at 1.2 (normalised) = ", flat(muscle::active_force_length(vec({1.2}))));
  // beyond the maximum shortening speed the force factor is clamped at zero
  out.writeln("force-velocity at -1, -1.5, -3 = ", flat(muscle::force_velocity(vec({-1.0, -1.5, -3.0}))));
}
```

Output

```text
active force at 0.12 (metres by mistake) = [0.0045], at 1.2 (normalised) = [0.7937]
force-velocity at -1, -1.5, -3 = [0.0000, 0.0000, 0.0000]
```

A fiber of `0.12` m with an optimal length of `0.10` m is `1.2` optimal lengths. Typed as `0.12` it is read as a fiber far shorter than its optimum and the active force is almost zero (`0.0045`), while the correct `1.2` gives `0.7937`. Faster shortening than the maximum speed (`-1`) gives a force factor of exactly `0.0000`: the curve does not go negative, so a speed of `-1.5` or `-3` is not an error, but it carries no more information than `-1`.

## Where next

[Isometric force](isometric_force.md) puts the fibers in series with a tendon and finds where their forces balance, and [Biology 18: Tissue Mechanics](../bio_18_tissue.md) describes the passive stress of soft tissue more generally.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="isometric_force.html">
    <span class="label">Next</span>
    <span class="title">Isometric force</span>
  </a>
</div>
<!-- apiref:nav:end -->
