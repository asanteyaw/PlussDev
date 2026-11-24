# Turing instability

[Back to Biology 12: Patterns](../bio_12_patterns.md)

Two chemicals that react and diffuse can turn a uniform state into stripes or spots, if the inhibitor spreads much faster than the activator. This page decides, from the reaction rates and diffusion coefficients alone, whether a uniform state forms a pattern, and at what spatial scale.

## How it works

Near a uniform steady state a small ripple of wavenumber `k` grows or decays at a rate set by the Jacobian `J` of the reactions and the diffusion matrix `D`: it is the eigenvalue of `J - k^2 D` with the largest real part. A pattern forms when the state is stable without diffusion (trace of `J` negative, determinant positive) but a band of wavenumbers `k^2` has a positive growth rate. `turing_conditions(J, da, db)` checks all of that and returns the band edges `k2_lower` and `k2_upper`, and `growth_rate(J, da, db, k2)` returns the rate for any array of `k^2`. `Brusselator{a, b}.jacobian(like)` gives the Jacobian of the Brusselator reactions at their steady state, for lanes shaped like `like`.

## Brusselator

With `a = 1` the activator diffuses at `0.05` and the inhibitor at `1`. The program tests three values of the control parameter `b` and, where a pattern forms, prints the unstable band and the growth rates at five wavenumbers.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace turing = biology::spatial::turing;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // Brusselator with a = 1: an activator diffusing at 0.05 and an inhibitor at 1.0
  const auto like = vec({0.0});
  const auto da = vec({0.05}), db = vec({1.0});
  for (double b : {1.2, 1.8, 3.0}) {
    const turing::Brusselator model{.a = 1.0, .b = b};
    const auto jacobian = model.jacobian(like);
    const auto c = turing::turing_conditions(jacobian, da, db);
    out.writeln("b = ", b, ": stable without diffusion = ", flat(c.stable_without_diffusion.to(pluss::ScalarType::Float64)),
                ", pattern forms = ", flat(c.turing_unstable.to(pluss::ScalarType::Float64)));
    if (flat(c.turing_unstable)[0] > 0.0) {
      out.writeln("    unstable band in k^2: ", flat(c.k2_lower), " to ", flat(c.k2_upper), ", centre of the band k^2 = ", flat(c.k2_critical));
      const auto k2 = vec({0.0, 0.5, 2.0, 4.0, 8.0});
      out.writeln("    growth rate at k^2 = 0, 0.5, 2, 4, 8: ", flat(turing::growth_rate(jacobian, da, db, k2)));
    }
  }
}
```

Output

```text
b = 1.2000: stable without diffusion = [1.0000], pattern forms = [0.0000]
b = 1.8000: stable without diffusion = [1.0000], pattern forms = [1.0000]
    unstable band in k^2: [1.4792] to [13.5208], centre of the band k^2 = [7.5000]
    growth rate at k^2 = 0, 0.5, 2, 4, 8: [-0.1000, -0.3625, 0.1238, 0.2576, 0.2044]
b = 3.0000: stable without diffusion = [0.0000], pattern forms = [0.0000]
```

At `b = 1.2` the uniform state is stable and no pattern forms. At `b = 3.0` the state is already unstable without diffusion (an oscillation, not a stationary pattern), so the Turing test fails. At `b = 1.8` it is stable without diffusion but unstable to ripples with `k^2` between `1.4792` and `13.5208`: the growth rate is negative at `k^2 = 0` and `0.5` (`-0.1000` and `-0.3625`) and positive at `2`, `4` and `8` (`0.1238`, `0.2576`, `0.2044`), so stripes or spots of that scale will grow.

## How fast must the inhibitor be?

Diffusion-driven patterns need the inhibitor to spread faster than the activator. The program uses the Schnakenberg kinetics, {math}`\dot u = a - u + u^2 v` and {math}`\dot v = b - u^2 v` with {math}`a = 0.1` and {math}`b = 0.9`, whose uniform state is {math}`(a + b, b/(a+b)^2)`, and tests five diffusion ratios.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace turing = biology::spatial::turing;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // Schnakenberg kinetics (a = 0.1, b = 0.9): the activator diffuses at 1, the inhibitor faster. How fast must the inhibitor be?
  const turing::Schnakenberg model{.a = 0.1, .b = 0.9};
  const auto like = vec({0.0});
  const auto steady = model.steady_state();
  out.writeln("uniform state (u, v) = (", steady.first, ", ", steady.second, ")");
  for (double ratio : {2.0, 5.0, 10.0, 20.0, 40.0}) {
    const auto c = turing::turing_conditions(model.jacobian(like), vec({1.0}), vec({ratio}));
    const bool pattern = flat(c.turing_unstable.to(pluss::ScalarType::Float64))[0] > 0.5;
    if (pattern) out.writeln("inhibitor ", ratio, " times faster: pattern forms, unstable band k^2 in [", flat(c.k2_lower)[0], ", ", flat(c.k2_upper)[0], "]");
    else out.writeln("inhibitor ", ratio, " times faster: no pattern");
  }
}
```

Output

```text
uniform state (u, v) = (1.0000, 0.9000)
inhibitor 2.0000 times faster: no pattern
inhibitor 5.0000 times faster: no pattern
inhibitor 10.0000 times faster: pattern forms, unstable band k^2 in [0.2000, 0.5000]
inhibitor 20.0000 times faster: pattern forms, unstable band k^2 in [0.0740, 0.6760]
inhibitor 40.0000 times faster: pattern forms, unstable band k^2 in [0.0337, 0.7413]
```

The uniform state is `(1.0000, 0.9000)`. A ratio of `2` or `5` gives no pattern; from `10` on the band of unstable wavenumbers opens, `[0.2000, 0.5000]` at `10`, widening to `[0.0740, 0.6760]` at `20` and `[0.0337, 0.7413]` at `40`, so a faster inhibitor allows patterns of longer and shorter wavelength.

## The dispersion relation

The growth rate of a ripple of squared wavenumber {math}`k^2`, with the determinant of {math}`J - k^2 D` that decides its sign, shows which scale grows fastest. For three or more species `unstable_modes(J, d, k2)` counts the eigenvalues with a positive real part. The program evaluates the relation for a diffusion ratio of `20` and counts the unstable modes of a three-species system at three wavenumbers.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace turing = biology::spatial::turing;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the dispersion relation: the growth rate of a ripple of wavenumber k, for Schnakenberg kinetics with an inhibitor 20 times faster
  const turing::Schnakenberg model{.a = 0.1, .b = 0.9};
  const auto jac = model.jacobian(vec({0.0}));
  const auto k2 = vec({0.0, 0.1, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0});
  const auto da = vec({1.0}), db = vec({20.0});
  out.writeln("k^2                   ", flat(k2));
  out.writeln("growth rate           ", flat(turing::growth_rate(jac, da, db, k2)));
  out.writeln("det(J - k^2 D)        ", flat(turing::determinant_of_perturbation(jac, da, db, k2)));
  // for three or more species: how many eigenvalues of J - k^2 D have a positive real part. The third species is inert.
  const auto three = vec({0.8, 1.0, 0.0, -2.0, -1.2, 0.0, 0.0, 0.0, -1.0}).reshape({3, 3});
  const auto diffusion = vec({1.0, 20.0, 1.0});
  for (double k2 : {0.0, 0.3, 3.0})
    out.writeln("k^2 = ", k2, ": unstable modes = ", flat(turing::unstable_modes(three, diffusion, vec({k2}))));
}
```

Output

```text
k^2                   [0.0000, 0.1000, 0.2500, 0.5000, 1.0000, 2.0000, 4.0000, 8.0000]
growth rate           [-0.1000, 0.1238, 0.2626, 0.1384, -0.2869, -1.2453, -3.2231, -7.2117]
det(J - k^2 D)        [1.0000, -0.3000, -1.5000, -1.5000, 6.0000, 51.0000, 261.0000, 1161.0000]
k^2 = 0.0000: unstable modes = [0.0000]
k^2 = 0.3000: unstable modes = [1.0000]
k^2 = 3.0000: unstable modes = [0.0000]
```

The growth rate is negative at {math}`k^2 = 0` (`-0.1000`), positive from `0.1` to `0.5` with a maximum of `0.2626` at `0.25`, and negative again from `1`. The determinant is negative exactly where the growth is positive. In the three-species system the number of unstable modes is `0` at {math}`k^2 = 0`, `1` at {math}`0.3` and `0` at {math}`3`: the instability is confined to a band, as for two species.

## Decisions

- Test with `turing_conditions` first: it tells you whether a pattern forms and over which band of {math}`k^2`, so you do not have to simulate.
- Look at `stable_without_diffusion` as well as `turing_unstable`: a state that is unstable without diffusion oscillates, and no choice of diffusion coefficients will make it a Turing pattern.
- Scan a parameter (such as `b` or the diffusion ratio) in one call by putting it on a lane of the Jacobian and of `da` and `db`.

## Pitfalls

A pattern needs the inhibitor to diffuse faster, the growth rate is a function of {math}`k^2` and not of {math}`k`, and the state must be stable without diffusion. The program breaks each of these in turn.

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

namespace turing = biology::spatial::turing;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto like = vec({0.0});
  const turing::Brusselator model{.a = 1.0, .b = 1.8};
  const auto jac = model.jacobian(like);
  // swapping the diffusion coefficients (activator faster than inhibitor) removes the pattern
  for (auto [da, db] : std::vector<std::pair<double, double>>{{0.05, 1.0}, {1.0, 0.05}}) {
    const auto c = turing::turing_conditions(jac, vec({da}), vec({db}));
    out.writeln("activator ", da, ", inhibitor ", db, ": pattern forms = ", flat(c.turing_unstable.to(pluss::ScalarType::Float64)));
  }
  // growth_rate takes k squared: a wavelength 2 pi / k = 2 pi / 1.5 is k^2 = 2.1, not k = 1.5
  const double k = 1.5;
  out.writeln("growth rate with k^2 = k (wrong) and k^2 = k*k: ", flat(turing::growth_rate(jac, vec({0.05}), vec({1.0}), vec({k, k * k}))));
  // a Hopf-unstable state (trace of J positive) is not a Turing instability
  const turing::Brusselator hopf{.a = 1.0, .b = 3.0};
  const auto c = turing::turing_conditions(hopf.jacobian(like), vec({0.05}), vec({1.0}));
  out.writeln("b = 3: stable without diffusion = ", flat(c.stable_without_diffusion.to(pluss::ScalarType::Float64)), ", pattern forms = ", flat(c.turing_unstable.to(pluss::ScalarType::Float64)));
}
```

Output

```text
activator 0.0500, inhibitor 1.0000: pattern forms = [1.0000]
activator 1.0000, inhibitor 0.0500: pattern forms = [0.0000]
growth rate with k^2 = k (wrong) and k^2 = k*k: [0.0070, 0.1596]
b = 3: stable without diffusion = [0.0000], pattern forms = [0.0000]
```

Swapping the diffusion coefficients turns the pattern off (`pattern forms = 0.0000`) even though the reactions are the same. Passing {math}`k = 1.5` where {math}`k^2` is wanted gives a growth rate of `0.0070` instead of `0.1596`, so the two differ by more than a factor of twenty. At `b = 3` the state is unstable without diffusion and the Turing test reports no pattern.

## Where next

[Biology 14: Gene Circuits](../bio_14_gene_circuits.md) studies instabilities that make oscillations in time rather than patterns in space, and [Biology 19: Diffusion](../bio_19_diffusion.md) describes how the spreading itself works.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
