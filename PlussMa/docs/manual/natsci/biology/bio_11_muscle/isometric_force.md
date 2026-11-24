# Isometric force

[Back to Biology 11: Muscle](../bio_11_muscle.md)

A real muscle is in series with a tendon, and the length of the whole muscle-tendon unit is shared between them. When the length is held fixed, the fibers and tendon settle where their forces balance. This page finds that balance for a range of lengths and three activation levels.

## How it works

`muscle::Params` describes the muscle: its maximum force, optimal fiber length, tendon slack length and the other constants of the model. `isometric_equilibrium(activation, mtu_length, params)` finds, for every pair of activation and muscle-tendon length, the tendon force at which the tendon pulls exactly as hard as the fibers push, and returns the `force` in newtons, the normalised fiber length and the tendon length. The solution is found for every lane at once, so a whole length-force curve at several activations is a single call.

## A length-force curve

The muscle can produce `1000` N, with fibers of optimal length `0.10` m and a tendon of slack length `0.20` m. The program holds the whole unit at seven lengths from `0.28` to `0.34` m and solves for the force at activations `0.2`, `0.5` and `1.0`, and prints the fiber and tendon lengths at full activation.

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
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // a muscle that can produce 1000 N, with 0.10 m optimal fibers and a 0.20 m tendon at rest
  muscle::Params muscle_params;
  muscle_params.max_isometric_force = 1000.0;
  muscle_params.optimal_fiber_length = 0.10;
  muscle_params.tendon_slack_length = 0.20;
  // muscle-tendon lengths from 0.28 m to 0.34 m, at three activations: 20%, 50% and 100%
  const auto length = vec({0.28, 0.29, 0.30, 0.31, 0.32, 0.33, 0.34});
  out.writeln("muscle-tendon length (m)   ", flat(length));
  for (double a : {0.2, 0.5, 1.0}) {
    const auto activation = length.mul(0.0).add(a);
    const auto eq = muscle::isometric_equilibrium(activation, length, muscle_params);
    out.writeln("force (N), activation ", a, "  ", flat(eq.force));
  }
  const auto full = muscle::isometric_equilibrium(length.mul(0.0).add(1.0), length, muscle_params);
  out.writeln("fiber length / optimal, full activation ", flat(full.norm_fiber_length));
  out.writeln("tendon length (m), full activation      ", flat(full.tendon_length));
}
```

Output

```text
muscle-tendon length (m)   [0.280, 0.290, 0.300, 0.310, 0.320, 0.330, 0.340]
force (N), activation 0.200  [163.876, 187.550, 211.032, 224.606, 226.696, 236.491, 285.353]
force (N), activation 0.500  [385.820, 442.643, 494.209, 521.939, 500.435, 448.152, 424.637]
force (N), activation 1.000  [723.833, 850.978, 950.765, 1018.549, 986.461, 848.788, 695.160]
fiber length / optimal, full activation [0.716, 0.809, 0.904, 1.001, 1.103, 1.209, 1.318]
tendon length (m), full activation      [0.208, 0.209, 0.210, 0.210, 0.210, 0.209, 0.208]
```

At full activation the force rises from `723.833` N at `0.28` m to a peak of `1018.549` N at `0.31` m, where the fibers are at their optimal length (`1.001`), and falls again to `695.160` N at `0.34` m. A lower activation scales the curve down: `0.5` gives about half the force at the peak (`521.939` N). At the lowest activation and the longest length, passive tissue adds force (`285.353` N at `0.34` m, more than at `0.33` m). The tendon is stiff: its length stays between `0.208` and `0.210` m, so nearly all of the change in length goes to the fibers.

## Pennation

In a pennated muscle the fibers are at an angle to the tendon, so only part of their force acts along it, and the fibers are longer than the muscle for a given length. `Params::pennation_at_optimal` sets the angle at the optimal fiber length. The program compares `0`, `20` and `40` degrees at five lengths.

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
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // muscles that differ only in pennation angle (0, 20 and 40 degrees at the optimal length): isometric force at full activation over five lengths
  const auto length = vec({0.27, 0.29, 0.31, 0.33, 0.35});
  out.writeln("muscle-tendon length (m)  ", flat(length));
  for (double degrees : {0.0, 20.0, 40.0}) {
    muscle::Params p; p.max_isometric_force = 1000.0; p.optimal_fiber_length = 0.10; p.tendon_slack_length = 0.20;
    p.pennation_at_optimal = degrees * 3.141592653589793 / 180.0;
    const auto eq = muscle::isometric_equilibrium(length.mul(0.0).add(1.0), length, p);
    out.writeln("pennation ", degrees, " deg: force (N) ", flat(eq.force));
  }
}
```

Output

```text
muscle-tendon length (m)  [0.27, 0.29, 0.31, 0.33, 0.35]
pennation 0.00 deg: force (N) [458.54, 850.98, 1018.55, 848.79, 646.62]
pennation 20.00 deg: force (N) [623.26, 851.26, 960.04, 738.97, 656.88]
pennation 40.00 deg: force (N) [652.83, 800.58, 726.33, 572.58, 809.24]
```

At the peak length `0.31` m the parallel-fibered muscle (`0` degrees) produces `1018.55` N, the `20` degree muscle `960.04` N and the `40` degree muscle `726.33` N. A larger angle lowers the peak and changes the shape of the curve: at `40` degrees it is no longer a single hump, and the force rises again at `0.35` m (`809.24` N).

## Tendon length

At a fixed total length, a longer tendon leaves shorter fibers. `Params::tendon_slack_length` may be an array, so three muscles that differ only in this length are one call. The program prints the force and the fiber and tendon lengths at a total length of `0.32` m.

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
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // tendon slack length trades fiber length against tendon length at the same total length: 0.15 m, 0.20 m and 0.25 m, total 0.32 m, full activation
  const auto length = vec({0.32, 0.32, 0.32});
  const auto activation = vec({1.0, 1.0, 1.0});
  muscle::Params p; p.max_isometric_force = 1000.0; p.optimal_fiber_length = 0.10;
  p.tendon_slack_length = vec({0.15, 0.20, 0.25});
  const auto eq = muscle::isometric_equilibrium(activation, length, p);
  out.writeln("tendon slack length (m)   ", flat(vec({0.15, 0.20, 0.25})));
  out.writeln("force (N)                 ", flat(eq.force));
  out.writeln("fiber length / optimal    ", flat(eq.norm_fiber_length));
  out.writeln("tendon length (m)         ", flat(eq.tendon_length));
}
```

Output

```text
tendon slack length (m)   [0.150, 0.200, 0.250]
force (N)                 [1244.377, 986.461, 409.703]
fiber length / optimal    [1.619, 1.103, 0.624]
tendon length (m)         [0.158, 0.210, 0.258]
```

With a short tendon (`0.15` m) the fibers are stretched to `1.619` times their optimal length and the force is `1244.377` N, mostly passive; with the standard `0.20` m the fibers are at `1.103` and the force `986.461` N; with a long tendon (`0.25` m) the fibers are short, `0.624`, and the force is only `409.703` N.

## Decisions

- Use `isometric_equilibrium` for a muscle held at fixed length; for a muscle that moves you need the time-dependent model of the muscle chapter's later pages.
- Describe a muscle by its `Params`: set the maximum force, optimal fiber length and tendon slack length first, and add `pennation_at_optimal` only for pennated muscles.
- Put several muscles, activations or lengths on lanes (arrays in `Params`, activation and length) and make one call instead of a loop.

## Pitfalls

Activation is a fraction between zero and one and the length is the whole unit in metres. The program passes an activation as a percentage, and a length in centimetres.

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
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  muscle::Params p; p.max_isometric_force = 1000.0; p.optimal_fiber_length = 0.10; p.tendon_slack_length = 0.20;
  const auto length = vec({0.31, 0.31, 0.31});
  // activation is a fraction between 0 and 1: 50 (percent) is rejected, 0.5 is half
  try { out.writeln("activation 50: ", flat(muscle::isometric_equilibrium(vec({50.0}), vec({0.31}), p).force)); }
  catch (const pluss::Error &e) { out.writeln("activation 50: ", e.message()); }
  out.writeln("activation 0.5: ", flat(muscle::isometric_equilibrium(vec({0.5}), vec({0.31}), p).force), " N");
  // the length is the whole muscle-tendon unit in metres: 31 (centimetres by mistake) leaves the fibers absurdly long and the force is only the passive pull
  out.writeln("force at 0.31 m and at 31 (cm by mistake): ", flat(muscle::isometric_equilibrium(vec({1.0, 1.0}), vec({0.31, 31.0}), p).force), " N");
}
```

Output

```text
activation 50: natsci: isometric_equilibrium: activation must lie between 0 and 1
activation 0.5: [521.939] N
force at 0.31 m and at 31 (cm by mistake): [1018.549, 5000.000] N
```

An activation of `50` is rejected; `0.5` gives about half the peak force, `521.939` N. A length of `31` instead of `0.31` is not rejected, but the result, `5000.000` N, is just the upper bound of the search (five times the maximum force) and means the length is far outside the range where the tendon and fibers can balance; check units when a force comes out at exactly that number.

## Where next

[Force curves](force_curves.md) describes the curves this page combines, and [Biology 13: Blood Flow](../bio_13_blood_flow.md) moves from the muscle to the vessels it squeezes.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="force_curves.html">
    <span class="label">Previous</span>
    <span class="title">Force curves</span>
  </a>
</div>
<!-- apiref:nav:end -->
