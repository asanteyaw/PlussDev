# Stress and stretch

[Back to Biology 18: Tissue Mechanics](../bio_18_tissue.md)

Soft tissue can be stretched to several times its length, and its stress is not proportional to the strain. Hyperelastic models describe it with a strain-energy function of the stretch. This page computes the stress in a stretched solid for the standard models, shows the strong stiffening of soft tissue, and the stresses of simple shear.


## How it works

A hyperelastic solid is defined by a strain-energy function {math}`W` of the deformation, and the stress is its derivative. The models `neo_hookean`, `mooney_rivlin`, `fung` and `gent` are built from a few constants, and `uniaxial_stress`, `equibiaxial_stress` and `simple_shear` evaluate the stress for the deformation you impose, as a function of the stretch ratio {math}`\lambda` (new length over old). `ogden_uniaxial` works from principal stretches instead. Every function takes arrays of stretches, so a whole stress-stretch curve is one call.

## Rubber-like solids

A hyperelastic solid has a strain-energy function of the invariants of the deformation. `tissue::neo_hookean(mu)` has {math}`W = \tfrac{\mu}{2}(I_1 - 3)` and `tissue::mooney_rivlin(c10, c01)` adds a second term. Stretching by a factor {math}`\lambda` in one direction gives the Cauchy stress {math}`\sigma = \mu(\lambda^2 - 1/\lambda)` for the neo-Hookean solid, which `tissue::uniaxial_stress(model, lambda)` evaluates; `tissue::equibiaxial_stress` pulls in two directions at once. The program compares a neo-Hookean solid of modulus {math}`1` with a Mooney-Rivlin solid of the same initial modulus.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace tissue = biology::biomechanics::tissue;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // stress in a rubber-like solid pulled to stretch ratios 1 to 3 (Cauchy stress in units of the shear modulus 1): the neo-Hookean and Mooney-Rivlin models
  const auto lambda = vec({1.0, 1.2, 1.5, 2.0, 2.5, 3.0});
  out.writeln("stretch                      ", flat(lambda));
  out.writeln("neo-Hookean, uniaxial        ", flat(tissue::uniaxial_stress(tissue::neo_hookean(1.0), lambda)));
  out.writeln("Mooney-Rivlin, uniaxial      ", flat(tissue::uniaxial_stress(tissue::mooney_rivlin(0.3, 0.2), lambda)));
  out.writeln("neo-Hookean, equibiaxial     ", flat(tissue::equibiaxial_stress(tissue::neo_hookean(1.0), lambda)));
}
```

Output

```text
stretch                      [1.0000, 1.2000, 1.5000, 2.0000, 2.5000, 3.0000]
neo-Hookean, uniaxial        [0.0000, 0.6067, 1.5833, 3.5000, 5.8500, 8.6667]
Mooney-Rivlin, uniaxial      [0.0000, 0.5662, 1.3722, 2.8000, 4.4460, 6.3556]
neo-Hookean, equibiaxial     [0.0000, 0.9577, 2.0525, 3.9375, 6.2244, 8.9877]
```

For a stretch of `2` the neo-Hookean solid carries a stress of `3.5000` in uniaxial and `3.9375` in equibiaxial tension, in units of its modulus. The Mooney-Rivlin solid has the same stiffness at small strain but is softer at large stretch (`2.8000` at a stretch of `2`).

## Stiffening of soft tissue

Arteries and skin stiffen sharply as they are stretched. The exponential Fung model `tissue::fung(c, b)` has {math}`W = \tfrac{c}{2b}(e^{b(I_1 - 3)} - 1)` and the Gent model `tissue::gent(mu, Jm)` limits the extension of the molecular chains. `tissue::initial_shear_modulus(model, like)` gives the stiffness at small strain. The program prints the moduli of a Fung and a Gent model and their stresses at stretches from `1.0` to `1.4`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace tissue = biology::biomechanics::tissue;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // soft tissue stiffens sharply: the exponential Fung model and the limiting-chain Gent model, with their small-strain shear moduli printed first
  const auto fung = tissue::fung(0.5, 5.0), gent = tissue::gent(1.0, 2.0), neo = tissue::neo_hookean(1.0);
  const auto like = vec({0.0});
  out.writeln("initial shear modulus: Fung ", flat(tissue::initial_shear_modulus(fung, like)), ", Gent ", flat(tissue::initial_shear_modulus(gent, like)), ", neo-Hookean ", flat(tissue::initial_shear_modulus(neo, like)));
  const auto lambda = vec({1.0, 1.1, 1.2, 1.3, 1.4});
  out.writeln("stretch             ", flat(lambda));
  out.writeln("Fung                ", flat(tissue::uniaxial_stress(fung, lambda)));
  out.writeln("Gent                ", flat(tissue::uniaxial_stress(gent, lambda)));
}
```

Output

```text
initial shear modulus: Fung [2.5000], Gent [1.0000], neo-Hookean [1.0000]
stretch             [1.0000, 1.1000, 1.2000, 1.3000, 1.4000]
Fung                [0.0000, 0.8661, 2.5853, 7.2142, 21.7335]
Gent                [0.0000, 0.3052, 0.6408, 1.0395, 1.5461]
```

The Fung model of soft tissue stiffens sharply: its stress rises by a factor of twenty-five between stretches of `1.1` and `1.4`, from `0.8661` to `21.7335`, against `1.5461` for the Gent solid. Their small-strain moduli are `2.5000` and `1.0000`.

## Shear and the Ogden model

In simple shear by an amount {math}`\gamma` a neo-Hookean solid has a shear stress {math}`\mu\gamma` and also a normal stress difference {math}`\mu\gamma^2`, the Poynting effect, which `tissue::simple_shear(model, gamma)` returns. The Ogden model `tissue::ogden_uniaxial(lambda, mu, alpha)` fits a wide range of materials with a few terms. The program prints the shear and the normal stresses, and a three-term Ogden fit.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace tissue = biology::biomechanics::tissue;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // simple shear: shear stress and the normal-stress difference (the Poynting effect) against the amount of shear, for a neo-Hookean solid with modulus 1
  const auto gamma = vec({0.1, 0.25, 0.5, 1.0, 2.0});
  const auto s = tissue::simple_shear(tissue::neo_hookean(1.0), gamma);
  out.writeln("amount of shear              ", flat(gamma));
  out.writeln("shear stress                 ", flat(s.shear_stress));
  out.writeln("normal stress difference     ", flat(s.normal_difference));
  // the Ogden model with three terms in uniaxial and equibiaxial tension
  const auto mu = vec({1.2, 0.02, -0.01}), alpha = vec({2.0, 8.0, -2.0});
  const auto lam = vec({1.2, 1.5, 2.0});
  out.writeln("Ogden uniaxial at stretch 1.2, 1.5, 2   ", flat(tissue::ogden_uniaxial(lam, mu, alpha)));
  out.writeln("Ogden small-strain shear modulus        ", flat(tissue::ogden_shear_modulus(mu, alpha)));
}
```

Output

```text
amount of shear              [0.1000, 0.2500, 0.5000, 1.0000, 2.0000]
shear stress                 [0.1000, 0.2500, 0.5000, 1.0000, 2.0000]
normal stress difference     [0.0100, 0.0625, 0.2500, 1.0000, 4.0000]
Ogden uniaxial at stretch 1.2, 1.5, 2   [0.8094, 2.4192, 9.3363]
Ogden small-strain shear modulus        [1.2900]
```

In simple shear the shear stress equals the amount of shear for the neo-Hookean solid (`0.1000` to `2.0000`), and the normal stress difference is its square (`0.0100` to `4.0000`): the Poynting effect, which grows faster than the shear. The three-term Ogden fit gives `0.8094`, `2.4192` and `9.3363` at stretches `1.2`, `1.5` and `2`, with a small-strain shear modulus of `1.2900`.

## Decisions

- Use neo-Hookean when you only need a modulus, Mooney-Rivlin for a rubber that softens slightly, and Fung or Gent for tissue that stiffens: Fung has no limit, Gent has a limiting stretch.
- Compare models by their small-strain shear modulus (`initial_shear_modulus`) first, so that differences at large stretch are real and not a mismatch of scale.
- Use the deformation that matches your experiment: uniaxial for a strip, equibiaxial for a membrane, simple shear for a block.

## Pitfalls

A stretch is a ratio of lengths and must be positive, the Gent model has a maximum stretch, and a uniaxial test is not a biaxial one. The program tries each.

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
namespace tissue = biology::biomechanics::tissue;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a stretch ratio is the new length over the old one: 1.2, not 20 (percent) and not 0.2 (strain)
  const auto neo = tissue::neo_hookean(1.0);
  out.writeln("stress at stretch 1.2, 0.2 (strain by mistake), 20 (percent by mistake): ", flat(tissue::uniaxial_stress(neo, vec({1.2, 0.2, 20.0}))));
  // a stretch of zero or less has no meaning
  try { out.writeln("stretch 0 and -1: ", flat(tissue::uniaxial_stress(neo, vec({0.0, -1.0})))); }
  catch (const pluss::Error &e) { out.writeln("stretch 0 and -1: ", e.message()); }
  // the Gent model has a limiting stretch: beyond it no stress is defined
  const auto gent = tissue::gent(1.0, 2.0);
  out.writeln("Gent stress at stretch 1.5: ", flat(tissue::uniaxial_stress(gent, vec({1.5}))));
  try { out.writeln("Gent stress at stretch 2.5: ", flat(tissue::uniaxial_stress(gent, vec({2.5})))); }
  catch (const pluss::Error &e) { out.writeln("Gent stress at stretch 2.5: ", e.message()); }
  // uniaxial and equibiaxial pulls are not interchangeable
  out.writeln("stretch 2: uniaxial, equibiaxial = ", std::vector<double>{flat(tissue::uniaxial_stress(neo, vec({2.0})))[0], flat(tissue::equibiaxial_stress(neo, vec({2.0})))[0]});
}
```

Output

```text
stress at stretch 1.2, 0.2 (strain by mistake), 20 (percent by mistake): [0.6067, -4.9600, 399.9500]
stretch 0 and -1: natsci: uniaxial_stress: stretch ratios must be positive
Gent stress at stretch 1.5: [2.2353]
Gent stress at stretch 2.5: natsci: gent: the deformation exceeds the limiting extensibility (I1 - 3 must not exceed Jm)
stretch 2: uniaxial, equibiaxial = [3.5000, 3.9375]
```

A stretch of `1.2` is a twenty percent extension; typing `0.2` (the strain) is a compression to a fifth of the length and gives `-4.9600`, and `20` (the percentage) gives `399.9500`. A stretch of zero or less is rejected. The Gent solid with a limit of `2` has stress `2.2353` at `1.5`, but at `2.5` the chains cannot extend that far and `gent` reports it. For the same stretch of `2` the uniaxial and equibiaxial stresses differ (`3.5000` and `3.9375`).

## Where next

[Biology 13: Blood Flow](../bio_13_blood_flow.md) uses the wall stiffness this page describes to find the speed of the pressure wave, and [Biology 11: Muscle](../bio_11_muscle.md) describes the active tissue that these passive models sit alongside.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
