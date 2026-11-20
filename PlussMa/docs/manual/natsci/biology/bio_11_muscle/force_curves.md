<!-- generated from force_curves.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="isometric_force.html">
    <span class="label">Next</span>
    <span class="title">Isometric force</span>
  </a>
</div>
<!-- apiref:nav:end -->
