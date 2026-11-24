# Bisubstrate, mechanism engine and allostery

[Back to Biology 43: Enzyme Mechanisms](../bio_43_mechanisms.md)

Many enzymes bind two substrates, many are slowed by their own substrate, and many are cooperative. You want to predict the rate of such an enzyme from the mechanism, and to tell mechanisms apart from rate measurements at different substrate levels. This page gives the initial-rate laws of the ordered, random and ping-pong two-substrate mechanisms and the diagnostic that separates them, computes the steady-state rate of any mechanism written as a list of steps, and describes a cooperative enzyme with the concerted allosteric model.

## How it works

The code is in `pluss::ma::natsci::biology::systems::enzymes` (`enzymes::` below). The rapid-equilibrium ordered, random and ping-pong two-substrate laws are `enzymes::ordered_bibi_rapid(A, B, vmax, Kia, Kb)`, `enzymes::random_bibi_rapid(A, B, vmax, Ka, Kb, alpha)` and `enzymes::pingpong_bibi(A, B, vmax, KmA, KmB)`. A double-reciprocal plot of {math}`1/v` against {math}`1/A` at several fixed {math}`B` gives lines that meet for ordered and random mechanisms (the slope depends on {math}`B`) and are parallel for ping-pong (the slope does not).

For any other mechanism you write the enzyme states and the steps between them. An `enzymes::Step` has the state it leaves and the state it reaches (`from`, `to`), a rate constant `k`, the index of the concentration that multiplies it (`species`, or `-1` for a first-order step) and `product_sign`, `+1` for a step that releases product and `-1` for one that takes it up. An `enzymes::Mechanism` has the number of states and the steps. `enzymes::steady_state(mechanism, concentrations)` returns the fraction of the enzyme in each state, the solution of {math}`Q\pi = 0` with {math}`\sum\pi = 1` for the rate matrix {math}`Q`, and `enzymes::turnover(mechanism, concentrations)` the net product flux per enzyme. Concentrations carry the species on the last axis, and leading axes are independent lanes.

For a cooperative enzyme with {math}`m` identical sites, the concerted model gives an active and an inactive form that bind the substrate with dissociation constants {math}`K_A` and {math}`K_I`, with the inactive form favoured by {math}`\beta\varepsilon` in units of {math}`kT` when no substrate is bound. The probability of the active form is

```{math}
p_{\mathrm{active}} = \frac{(1 + c/K_A)^m}{(1 + c/K_A)^m + e^{\beta\varepsilon\,\text{(sign as in the call)}}(1 + c/K_I)^m}
```

given by `enzymes::mwc_active_probability(c, m, K_A, K_I, beta_eps)`, and the velocity, from the sites that are occupied in the active form, by `enzymes::mwc_enzyme_velocity(c, m, vmax, K_A, K_I, beta_eps)`. `enzymes::mwc_characteristics(m, K_A, K_I, beta_eps)` returns the leakiness (the active probability with no substrate), the maximum, the dynamic range, the half-maximal concentration `ec50` and the effective Hill coefficient `hill`.

## Telling two-substrate mechanisms apart

The program computes the slope of {math}`1/v` against {math}`1/A` between {math}`A = 1` and {math}`A = 2` at three levels of the second substrate {math}`B`, for an ordered, a random and a ping-pong mechanism.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace enzymes = natsci::biology::systems::enzymes;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static enzymes::Step step(int from, int to, double k, int species = -1, double sign = 0.0) { enzymes::Step s; s.from = from; s.to = to; s.k = k; s.species = species; s.product_sign = sign; return s; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // initial-rate formulas for two substrates A and B. A double-reciprocal plot of 1/v against 1/A at fixed B has slope
  // (Kia Kb / Vmax) / B ... for ordered and random mechanisms the slope depends on B (lines intersect), for ping-pong it does not (parallel lines)
  const double vmax = 10.0;
  const auto slope = [&](auto v, double b) {                    // slope of 1/v against 1/A between A = 1 and A = 2, at substrate B = b
    const auto a = vec({1.0, 2.0});
    const auto inv = flat(v(a, vec({b, b})).reciprocal());
    return (inv[0] - inv[1]) / (1.0 / 1.0 - 1.0 / 2.0);
  };
  const auto ordered = [&](const pluss::Series &a, const pluss::Series &b) { return enzymes::ordered_bibi_rapid(a, b, vmax, 2.0, 3.0); };
  const auto random = [&](const pluss::Series &a, const pluss::Series &b) { return enzymes::random_bibi_rapid(a, b, vmax, 2.0, 3.0, 0.5); };
  const auto pingpong = [&](const pluss::Series &a, const pluss::Series &b) { return enzymes::pingpong_bibi(a, b, vmax, 1.5, 4.0); };
  for (double b : {1.0, 4.0, 16.0})
    out.writeln("B = ", b, ": slope of 1/v against 1/A: ordered ", slope(ordered, b), ", random ", slope(random, b), ", ping-pong ", slope(pingpong, b));
}
```

Output

```text
B = 1.0000: slope of 1/v against 1/A: ordered 0.6000, random 0.4000, ping-pong 0.1500
B = 4.0000: slope of 1/v against 1/A: ordered 0.1500, random 0.1750, ping-pong 0.1500
B = 16.0000: slope of 1/v against 1/A: ordered 0.0375, random 0.1188, ping-pong 0.1500
```

For the ordered mechanism the slope is `0.6000`, `0.1500` and `0.0375`: it falls in proportion to {math}`1/B`, so the lines of the double-reciprocal plot meet. For the random mechanism it also depends on {math}`B` (`0.4000`, `0.1750`, `0.1188`), while for ping-pong it is `0.1500` at every {math}`B`: parallel lines. A set of parallel lines in the measured data points to ping-pong.

## A mechanism as a list of steps

The program writes the Michaelis-Menten mechanism as two enzyme states and three steps and solves it for six substrate concentrations in one call, comparing the turnover with {math}`k_{cat}S/(K_m + S)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace enzymes = natsci::biology::systems::enzymes;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static enzymes::Step step(int from, int to, double k, int species = -1, double sign = 0.0) { enzymes::Step s; s.from = from; s.to = to; s.k = k; s.species = species; s.product_sign = sign; return s; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a mechanism is a list of steps between enzyme states. States: 0 free enzyme E, 1 complex ES. Steps: E + S -> ES (k1 = 10 per uM per s, species 0 = the substrate),
  // ES -> E + S (k-1 = 1), ES -> E + P (kcat = 0.5, releases product)
  const enzymes::Mechanism mm{2, {step(0, 1, 10.0, 0), step(1, 0, 1.0), step(1, 0, 0.5, -1, 1.0)}};
  const auto s = vec({0.02, 0.05, 0.15, 0.5, 2.0, 10.0}).unsqueeze(-1);            // substrate concentrations, one lane each
  const auto pi = enzymes::steady_state(mm, s);
  out.writeln("substrate               ", flat(s));
  out.writeln("fraction of enzyme as ES ", flat(pi.select(-1, 1)));
  out.writeln("turnover per enzyme     ", flat(enzymes::turnover(mm, s)));
  const double km = (1.0 + 0.5) / 10.0;
  std::vector<double> formula; for (double c : flat(s)) formula.push_back(0.5 * c / (km + c));
  out.writeln("kcat S / (Km + S)       ", formula, "   with Km = (k-1 + kcat) / k1 = ", km);
}
```

Output

```text
substrate               [0.0200, 0.0500, 0.1500, 0.5000, 2.0000, 10.0000]
fraction of enzyme as ES [0.1176, 0.2500, 0.5000, 0.7692, 0.9302, 0.9852]
turnover per enzyme     [0.0588, 0.1250, 0.2500, 0.3846, 0.4651, 0.4926]
kcat S / (Km + S)       [0.0588, 0.1250, 0.2500, 0.3846, 0.4651, 0.4926]   with Km = (k-1 + kcat) / k1 = 0.1500
```

The fraction of the enzyme in the complex rises from `0.1176` to `0.9852` as the substrate rises, and is exactly `0.5000` at {math}`S = K_m = 0.15`. The turnover per enzyme, from the steps alone, is `0.0588`, `0.1250`, `0.2500`, `0.3846`, `0.4651` and `0.4926`, identical to the closed form with {math}`K_m = (k_{-1} + k_{cat})/k_1`, which the engine did not use.

## A dead-end complex

A second molecule of substrate can bind the complex and make a form that cannot turn over. The program adds two steps to the mechanism and finds the turnover from `0.05` to `20`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace enzymes = natsci::biology::systems::enzymes;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static enzymes::Step step(int from, int to, double k, int species = -1, double sign = 0.0) { enzymes::Step s; s.from = from; s.to = to; s.k = k; s.species = species; s.product_sign = sign; return s; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // add a second, dead-end step: ES + S -> ES2 (kon = 8) and back (koff = 2), a complex that does not turn over
  const enzymes::Mechanism inhibited{3, {step(0, 1, 10.0, 0), step(1, 0, 1.0), step(1, 0, 0.5, -1, 1.0), step(1, 2, 8.0, 0), step(2, 1, 2.0)}};
  const auto s = vec({0.05, 0.1, 0.1936, 0.3, 0.5, 1.0, 2.0, 5.0, 20.0}).unsqueeze(-1);
  out.writeln("substrate        ", flat(s));
  out.writeln("turnover         ", flat(enzymes::turnover(inhibited, s)));
  const auto pi = enzymes::steady_state(inhibited, s);
  out.writeln("fraction as ES2  ", flat(pi.select(-1, 2)));
  const auto v = flat(enzymes::turnover(inhibited, s));
  out.writeln("the rate peaks near ", flat(s)[std::max_element(v.begin(), v.end()) - v.begin()], " and falls at higher substrate (substrate inhibition)");
  out.writeln("closed form sqrt(Km Ksi) = ", std::sqrt(0.15 * 0.25), " for the optimum with Km = 0.15 and Ksi = koff/kon = 0.25");
}
```

Output

```text
substrate        [0.0500, 0.1000, 0.1936, 0.3000, 0.5000, 1.0000, 2.0000, 5.0000, 20.0000]
turnover         [0.1190, 0.1724, 0.1961, 0.1852, 0.1515, 0.0971, 0.0551, 0.0238, 0.0062]
fraction as ES2  [0.0476, 0.1379, 0.3038, 0.4444, 0.6061, 0.7767, 0.8815, 0.9510, 0.9876]
the rate peaks near 0.1936 and falls at higher substrate (substrate inhibition)
closed form sqrt(Km Ksi) = 0.1936 for the optimum with Km = 0.15 and Ksi = koff/kon = 0.25
```

The turnover rises to `0.1961` at {math}`S = 0.1936 = \sqrt{K_m K_{si}}`, the optimum of {math}`v = k_{cat}S/(K_m + S + S^2/K_{si})`, and then falls: it is `0.0971` at `1` and `0.0062` at `20`, where `0.9876` of the enzyme is in the dead-end form. The engine reproduces substrate inhibition without any rate law being written.

## A cooperative enzyme

A tetramer has an active form with {math}`K_A = 0.5` and an inactive form with {math}`K_I = 20`, and the inactive form is favoured by {math}`2\,kT` with no substrate. The program prints the active fraction and the velocity from `0.01` to `100`, and the summary numbers.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace enzymes = natsci::biology::systems::enzymes;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static enzymes::Step step(int from, int to, double k, int species = -1, double sign = 0.0) { enzymes::Step s; s.from = from; s.to = to; s.k = k; s.species = species; s.product_sign = sign; return s; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a tetrameric allosteric enzyme (m = 4 sites): active form binds substrate with K_A = 0.5, the inactive form with K_I = 20, and the
  // inactive form is favoured without substrate by beta eps = 2 kT. The velocity is that of the sites that are occupied in the active form
  const auto c = vec({0.01, 0.1, 0.3, 1.0, 3.0, 10.0, 100.0});
  out.writeln("substrate      ", flat(c));
  out.writeln("active fraction", flat(enzymes::mwc_active_probability(c, 4, 0.5, 20.0, -2.0)));
  out.writeln("velocity (Vmax = 1) ", flat(enzymes::mwc_enzyme_velocity(c, 4, 1.0, 0.5, 20.0, -2.0)));
  const auto ch = enzymes::mwc_characteristics(4, vec({0.5}), vec({20.0}), vec({-2.0}));
  out.writeln("leakiness ", flat(ch.leakiness)[0], ", maximum ", flat(ch.maximum)[0], ", EC50 ", flat(ch.ec50)[0], ", effective Hill coefficient ", flat(ch.hill)[0]);
  // no difference between the two forms: no cooperativity
  const auto flatc = enzymes::mwc_characteristics(4, vec({0.5}), vec({0.5}), vec({-2.0}));
  (void)flatc;
}
```

Output

```text
substrate      [0.0100, 0.1000, 0.3000, 1.0000, 3.0000, 10.0000, 100.0000]
active fraction[0.1276, 0.2157, 0.4552, 0.9002, 0.9946, 0.9998, 1.0000]
velocity (Vmax = 1) [0.0025, 0.0360, 0.1707, 0.6001, 0.8526, 0.9522, 0.9950]
leakiness 0.1192, maximum 1.0000, EC50 0.3924, effective Hill coefficient 1.8824
```

The velocity rises from `0.0025` to `0.9950` in an S-shaped curve: only `0.0360` at `0.1`, `0.6001` at `1.0`, `0.9522` at `10`. The leakiness is `0.1192` (the active fraction with no substrate), the half-maximal concentration is `0.3924` and the effective Hill coefficient `1.8824`, well above the `1` of a non-cooperative enzyme.

## What makes an enzyme more cooperative

The program varies the number of sites at fixed constants, and then the preference of the inactive form at four sites.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace enzymes = natsci::biology::systems::enzymes;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static enzymes::Step step(int from, int to, double k, int species = -1, double sign = 0.0) { enzymes::Step s; s.from = from; s.to = to; s.k = k; s.species = species; s.product_sign = sign; return s; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the effective Hill coefficient of an MWC enzyme rises with the number of sites and with the preference of the inactive form
  for (int m : {1, 2, 4, 8}) {
    const auto ch = enzymes::mwc_characteristics(m, vec({0.5}), vec({20.0}), vec({-2.0}));
    out.writeln("m = ", m, ": EC50 ", flat(ch.ec50)[0], ", Hill coefficient ", flat(ch.hill)[0], ", dynamic range ", flat(ch.dynamic_range)[0]);
  }
  for (double eps : {-1.0, -3.0, -6.0}) {
    const auto ch = enzymes::mwc_characteristics(4, vec({0.5}), vec({20.0}), vec({eps}));
    out.writeln("m = 4, beta eps = ", eps, ": EC50 ", flat(ch.ec50)[0], ", Hill coefficient ", flat(ch.hill)[0], ", leakiness ", flat(ch.leakiness)[0]);
  }
}
```

Output

```text
m = 1: EC50 3.5405, Hill coefficient 1.0000, dynamic range 0.7249
m = 2: EC50 1.1096, Hill coefficient 1.4344, dynamic range 0.8762
m = 4: EC50 0.3924, Hill coefficient 1.8824, dynamic range 0.8808
m = 8: EC50 0.1671, Hill coefficient 2.1681, dynamic range 0.8808
m = 4, beta eps = -1.0000: EC50 0.2460, Hill coefficient 1.6120, leakiness 0.2689
m = 4, beta eps = -3.0000: EC50 0.6174, Hill coefficient 2.1895, leakiness 0.0474
m = 4, beta eps = -6.0000: EC50 1.9637, Hill coefficient 2.8380, leakiness 0.0025
```

With one site the Hill coefficient is `1.0000` however the other constants are chosen; it rises with the number of sites (`1.4344`, `1.8824`, `2.1681` for `2`, `4`, `8`) and with the preference for the inactive form (`1.6120`, `2.1895`, `2.8380` for {math}`\beta\varepsilon` of `-1`, `-3`, `-6`). A stronger preference makes the enzyme less leaky (`0.2689`, `0.0474`, `0.0025`) and moves the half-maximal concentration up.

## Decisions

- Use the closed rate laws when the mechanism is one of the three classic ones and you want a fit; use the mechanism engine for anything with dead-end complexes, reverse steps or a mechanism you invented.
- Look at the pattern of lines in a double-reciprocal plot before you pick a mechanism: parallel lines point to ping-pong, crossing lines to a sequential mechanism.
- For a cooperative enzyme use the concerted model when the subunits switch together, and report `ec50` and `hill` rather than a single dissociation constant.

## Pitfalls

Two ways to get a wrong rate without being told. The ping-pong law takes its combined {math}`V_{max} = E_t k_1 k_2/(k_1 + k_2)`, not the faster of the two rate constants, and a step may name a concentration that the vector does not have.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace enzymes = natsci::biology::systems::enzymes;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
static enzymes::Step step(int from, int to, double k, int species = -1, double sign = 0.0) { enzymes::Step s; s.from = from; s.to = to; s.k = k; s.species = species; s.product_sign = sign; return s; }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // ping-pong takes apparent constants and the combined Vmax = Et kcat1 kcat2 / (kcat1 + kcat2), not the larger of the two rate constants
  const double et = 1.0, kcat1 = 6.0, kcat2 = 3.0;
  const auto a = vec({2.0}), b = vec({2.0});
  out.writeln("Vmax from the formula        = ", et * kcat1 * kcat2 / (kcat1 + kcat2));
  out.writeln("velocity with that Vmax      = ", flat(enzymes::pingpong_bibi(a, b, et * kcat1 * kcat2 / (kcat1 + kcat2), 1.0, 1.0))[0]);
  out.writeln("velocity with Vmax = Et kcat1 = ", flat(enzymes::pingpong_bibi(a, b, et * kcat1, 1.0, 1.0))[0], "  (too large by the factor ", (et * kcat1) / (et * kcat1 * kcat2 / (kcat1 + kcat2)), ")");
  // a step whose species index points past the concentration vector (here species 1, with a vector that holds only the substrate)
  const enzymes::Mechanism bad{2, {step(0, 1, 10.0, 1), step(1, 0, 1.0), step(1, 0, 0.5, -1, 1.0)}};
  try {
    out.writeln("turnover: ", flat(enzymes::turnover(bad, vec({0.5}).unsqueeze(0)))[0]);
  } catch (const pluss::Error &e) { out.writeln("error: ", e.message()); }
}
```

Output

```text
Vmax from the formula        = 2.0000
velocity with that Vmax      = 1.0000
velocity with Vmax = Et kcat1 = 3.0000  (too large by the factor 3.0000)
error: natsci: enzymes::rate_matrix: a step uses a species index beyond the concentration vector
```

With {math}`k_1 = 6` and {math}`k_2 = 3` the maximum rate is `2.0000`, and the velocity at {math}`A = B = 2` with unit constants is `1.0000`. Using {math}`E_t k_1` for {math}`V_{max}` gives `3.0000`, three times too large, and nothing complains because the number is a perfectly good rate. A step that refers to concentration `1` when only the substrate is given is rejected with an error that names the problem.

## Where next

[Biology 5: Enzymes](../bio_05_enzymes.md) gives the one-substrate rate laws and inhibition, [Biology 27: Enzyme Fitting](../bio_27_enzyme_fitting.md) fits those laws to progress curves, and [Biology 15: Binding](../bio_15_binding.md) treats cooperative binding without catalysis.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
