<!-- generated from isometric_force.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="force_curves.html">
    <span class="label">Previous</span>
    <span class="title">Force curves</span>
  </a>
</div>
<!-- apiref:nav:end -->
