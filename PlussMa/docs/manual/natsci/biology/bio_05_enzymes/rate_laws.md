# Rate laws

[Back to Biology 5: Enzymes](../bio_05_enzymes.md)

An enzyme's reaction rate depends on how much substrate there is and on whatever else binds to the enzyme. This page evaluates the Michaelis-Menten law and the standard ways a second molecule changes it: competitive, uncompetitive and mixed inhibition, inhibition by the substrate itself, and cooperative activation.

## How it works

Each law is a function of the substrate concentration, returning the rate at every concentration in the array. `michaelis_menten(s, vmax, km)` is `vmax s / (km + s)`: the rate rises with substrate and levels off at `vmax`, and `km` is the concentration at which the rate is half of `vmax`. An inhibitor at concentration `I` changes the apparent constants: a competitive inhibitor (`competitive_inhibition`, constant `Ki`) multiplies `km` by `1 + I/Ki`, so enough substrate still reaches `vmax`; an uncompetitive inhibitor (`uncompetitive_inhibition`, constant `Ki'`) divides both `vmax` and `km`; a mixed inhibitor (`mixed_inhibition`) has both constants. `substrate_inhibition` adds a `s^2/Ksi` term to the denominator, so the rate falls at high substrate, and `hill_activation(l, vmax, k_half, n)` gives the sigmoidal rise of a cooperative enzyme with `n` binding sites. The coefficients may be arrays as well, so a batch of enzymes is one call.

## Six laws at five concentrations

The program evaluates each law with `vmax = 10` and `km = 1` at substrate concentrations from `0.1` to `20`, with an inhibitor at `2` for the three inhibition laws.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace kinetics = biology::systems::kinetics;
namespace enzymes = biology::systems::enzymes;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto s = vec({0.1, 0.5, 1.0, 5.0, 20.0});
  const double vmax = 10.0, km = 1.0;
  out.writeln("substrate                   ", flat(s));
  out.writeln("Michaelis-Menten            ", flat(kinetics::michaelis_menten(s, vmax, km)));
  const auto inhibitor = s.mul(0.0).add(2.0);
  out.writeln("competitive (I = 2, Ki = 1) ", flat(kinetics::competitive_inhibition(s, inhibitor, vmax, km, 1.0)));
  out.writeln("uncompetitive               ", flat(enzymes::uncompetitive_inhibition(s, inhibitor, vmax, km, 1.0)));
  out.writeln("mixed (Ki = 1, Ki' = 4)     ", flat(enzymes::mixed_inhibition(s, inhibitor, vmax, km, 1.0, 4.0)));
  out.writeln("substrate inhibition        ", flat(enzymes::substrate_inhibition(s, vmax, km, 10.0)));
  out.writeln("Hill activation, n = 3      ", flat(kinetics::hill_activation(s, vmax, 1.0, 3.0)));
}
```

Output

```text
substrate                   [0.1000, 0.5000, 1.0000, 5.0000, 20.0000]
Michaelis-Menten            [0.9091, 3.3333, 5.0000, 8.3333, 9.5238]
competitive (I = 2, Ki = 1) [0.3226, 1.4286, 2.5000, 6.2500, 8.6957]
uncompetitive               [0.7692, 2.0000, 2.5000, 3.1250, 3.2787]
mixed (Ki = 1, Ki' = 4)     [0.3175, 1.3333, 2.2222, 4.7619, 6.0606]
substrate inhibition        [0.9083, 3.2787, 4.7619, 5.8824, 3.2787]
Hill activation, n = 3      [0.0100, 1.1111, 5.0000, 9.9206, 9.9988]
```

The plain law reaches `9.5238` at the highest concentration, near `vmax`. The competitive inhibitor lowers every rate but the high-substrate end is still approaching `10` (`8.6957`), while the uncompetitive inhibitor caps the rate at `vmax/(1 + I/Ki') = 3.3333`, which the column of `3.2787` at `20` is approaching. Mixed inhibition falls between them. Substrate inhibition is highest at the sampled concentration `5` (`5.8824`) and falls to `3.2787` at `20`, and the Hill law with `n = 3` is almost silent at `0.1` (`0.0100`) and almost saturated at `5` (`9.9206`).

<!-- addex:start -->
## Batches, reversible reactions and tight binders

The coefficients as well as the substrate may be arrays, so a collection of different enzymes is one call. For a reversible reaction `enzymes::reversible_michaelis_menten(s, p, vf, ks, vr, kp)` gives {math}`v = (V_f S/K_s - V_r P/K_p)/(1 + S/K_s + P/K_p)`, which changes sign when product has accumulated past equilibrium. For an inhibitor that binds so tightly that it removes a significant part of the enzyme, `enzymes::morrison(E, I, Ki)` returns the fraction of the activity left. The program evaluates all three.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace kinetics = biology::systems::kinetics;
namespace enzymes = biology::systems::enzymes;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a batch of three enzymes with different Vmax and Km, all at substrate 2: the coefficients are arrays too
  const auto s = vec({2.0, 2.0, 2.0});
  out.writeln("rate of three enzymes (Vmax 10, 20, 5; Km 1, 4, 0.5) = ", flat(kinetics::michaelis_menten(s, vec({10.0, 20.0, 5.0}), vec({1.0, 4.0, 0.5}))));
  // a reversible reaction: the rate changes sign as product builds up (Haldane: Keq = Vf Kp / (Vr Ks))
  const auto substrate = vec({5.0, 5.0, 5.0, 5.0});
  const auto product = vec({0.0, 5.0, 20.0, 50.0});
  out.writeln("product concentration                  ", flat(product));
  out.writeln("reversible Michaelis-Menten rate        ", flat(enzymes::reversible_michaelis_menten(substrate, product, 10.0, 2.0, 4.0, 4.0)));
  // tight-binding inhibition (Morrison): the fraction of the enzyme activity left when the inhibitor is comparable to the enzyme concentration
  out.writeln("fractional activity left (E = 1, I = 0.5, 1, 2, Ki = 0.1) = ", flat(enzymes::morrison(vec({1.0, 1.0, 1.0}), vec({0.5, 1.0, 2.0}), 0.1)));
}
```

Output

```text
rate of three enzymes (Vmax 10, 20, 5; Km 1, 4, 0.5) = [6.6667, 6.6667, 4.0000]
product concentration                  [0.0000, 5.0000, 20.0000, 50.0000]
reversible Michaelis-Menten rate        [7.1429, 4.2105, 0.5882, -1.5625]
fractional activity left (E = 1, I = 0.5, 1, 2, Ki = 0.1) = [0.5742, 0.2702, 0.0844]
```

Three enzymes with different {math}`V_{max}` and {math}`K_m` at the same substrate give `6.6667`, `6.6667` and `4.0000`, one call each. The reversible rate falls from `7.1429` with no product to `0.5882` at a product concentration of `20` and becomes negative, `-1.5625`, at `50`: the reaction runs backwards past the equilibrium at a product concentration of `25`. With `0.5`, `1` and `2` units of a tight-binding inhibitor with {math}`K_i = 0.1` the enzyme keeps `0.5742`, `0.2702` and `0.0844` of its activity.

## How switch-like a response is

The Hill coefficient {math}`n` sets how sharply the response turns on. `hill_activation(l, vmax, K, n)` and `hill_repression(l, vmax, K, n)` are mirror images, equal to half of {math}`V_{max}` at {math}`l = K` for every {math}`n`. The program prints both for {math}`n = 1, 2, 4` at five ligand levels, and uses `michaelis_constant(k_minus, kcat, k_plus)` to get {math}`K_m` from the elementary rates.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace kinetics = biology::systems::kinetics;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the Hill coefficient sets how switch-like the response is: activation and repression at ligand 0.25 ... 4 for K = 1
  const auto ligand = vec({0.25, 0.5, 1.0, 2.0, 4.0});
  out.writeln("ligand               ", flat(ligand));
  for (double n : {1.0, 2.0, 4.0}) {
    out.writeln("activation, n = ", n, "  ", flat(kinetics::hill_activation(ligand, 1.0, 1.0, n)));
    out.writeln("repression, n = ", n, "  ", flat(kinetics::hill_repression(ligand, 1.0, 1.0, n)));
  }
  // the Michaelis constant from the elementary rate constants: Km = (k_-1 + kcat) / k_1
  out.writeln("Km from k1 = 10, k-1 = 5, kcat = 5 = ", flat(kinetics::michaelis_constant(vec({5.0}), vec({5.0}), vec({10.0}))));
}
```

Output

```text
ligand               [0.2500, 0.5000, 1.0000, 2.0000, 4.0000]
activation, n = 1.0000  [0.2000, 0.3333, 0.5000, 0.6667, 0.8000]
repression, n = 1.0000  [0.8000, 0.6667, 0.5000, 0.3333, 0.2000]
activation, n = 2.0000  [0.0588, 0.2000, 0.5000, 0.8000, 0.9412]
repression, n = 2.0000  [0.9412, 0.8000, 0.5000, 0.2000, 0.0588]
activation, n = 4.0000  [0.0039, 0.0588, 0.5000, 0.9412, 0.9961]
repression, n = 4.0000  [0.9961, 0.9412, 0.5000, 0.0588, 0.0039]
Km from k1 = 10, k-1 = 5, kcat = 5 = [1.0000]
```

At the half-saturation point the response is `0.5000` whatever {math}`n` is. Away from it, a larger {math}`n` makes the change sharper: from ligand `0.5` to `2.0` activation goes from `0.3333` to `0.6667` for {math}`n = 1`, from `0.2000` to `0.8000` for {math}`n = 2` and from `0.0588` to `0.9412` for {math}`n = 4`. Repression is the mirror image. The constant {math}`K_m = (k_{-1} + k_{cat})/k_1 = 1.0000` for the rates given.

## Arguments that are checked

The constants of the rate laws must be positive. The program passes a zero {math}`K_m` to the Michaelis-Menten law, a negative inhibition constant, and a Hill coefficient of zero.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace kinetics = biology::systems::kinetics;
int main() {
  OutputEngine out;
  try {
    (void)kinetics::michaelis_menten(vec({1.0, 2.0}), 10.0, 0.0);
  } catch (const pluss::Error &e) {
    out.writeln("zero Km:        ", e.message());
  }
  try {
    (void)kinetics::competitive_inhibition(vec({1.0}), vec({1.0}), 10.0, 1.0, -2.0);
  } catch (const pluss::Error &e) {
    out.writeln("negative Ki:    ", e.message());
  }
  try {
    (void)kinetics::hill_activation(vec({1.0}), 10.0, 1.0, 0.0);
  } catch (const pluss::Error &e) {
    out.writeln("Hill n of zero: ", e.message());
  }
}
```

Output

```text
zero Km:         natsci: michaelis_menten: Km must be finite and strictly positive
negative Ki:     natsci: competitive_inhibition: Ki must be finite and strictly positive
Hill n of zero:  natsci: hill: n must be finite and strictly positive
```

Each call raises a `pluss::Error` whose message names the function and the constant that was not acceptable.

<!-- addex:end -->

## Decisions

- Use `michaelis_menten` for a single substrate with no regulation; reach for an inhibition law only when you have measured the inhibitor's effect on both `vmax` and `km`.
- Use `hill_activation` or `hill_repression` when the response is switch-like; the coefficient is an effective number, not necessarily a count of sites.
- Sample substrate over several orders of magnitude before choosing a law.

## Pitfalls

A rate law fitted on a narrow range can look the same as another. The program samples the Michaelis-Menten law and the substrate-inhibition law at substrate levels from `1` to `300`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace enzymes = biology::systems::enzymes;
namespace kinetics = biology::systems::kinetics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a rate law sampled only at low substrate looks like it saturates; substrate inhibition shows itself later
  const auto s = vec({1.0, 3.0, 10.0, 30.0, 100.0, 300.0});
  out.writeln("substrate            ", flat(s));
  out.writeln("Michaelis-Menten     ", flat(kinetics::michaelis_menten(s, 10.0, 1.0)));
  out.writeln("substrate inhibition ", flat(enzymes::substrate_inhibition(s, 10.0, 1.0, 20.0)));
}
```

Output

```text
substrate            [1.0000, 3.0000, 10.0000, 30.0000, 100.0000, 300.0000]
Michaelis-Menten     [5.0000, 7.5000, 9.0909, 9.6774, 9.9010, 9.9668]
substrate inhibition [4.8780, 6.7416, 6.2500, 3.9474, 1.6639, 0.6249]
```

At `1` and `3` the two laws are close (`5.0000` and `4.8780`, `7.5000` and `6.7416`), and a measurement over that range could not tell them apart. At `100` the plain law is at `9.9010` and the inhibited one has fallen to `1.6639`, and at `300` it is `0.6249`. Look at the high-substrate end before concluding that an enzyme saturates.

## Where next

[Progress curves](progress_curves.md) follows the substrate over time, and [Biology 15: Binding](../bio_15_binding.md) treats cooperative binding.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="progress_curves.html">
    <span class="label">Next</span>
    <span class="title">Progress curves</span>
  </a>
</div>
<!-- apiref:nav:end -->
