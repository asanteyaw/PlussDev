# Gelation

[Back to Chemistry 2: Polymers](../chem_02_polymers.md)

If a monomer has three reactive arms instead of two, the chains branch and at some conversion join into a single network that spans the whole sample: the sample turns from a liquid into a gel. This page finds the conversion at which that happens and how much of the sample is still free to dissolve beyond it.

## How it works

`step_growth::critical_conversion(f, g, r)` is the conversion of the `A` groups at the gel point for a monomer of functionality `f` reacting with a monomer of functionality `g`, with `r` the ratio of `A` to `B` groups (at most `1`). Beyond that conversion, `step_growth::sol_gel(pa, r, f, g)` returns the weight fraction of the sample that is still sol, finite molecules that dissolve, and the fraction that has joined the gel. Both take arrays for the conversion and the ratio.

## A three-armed and a two-armed monomer

The program mixes a three-armed monomer (`f = 3`) with a two-armed one (`g = 2`), first in equal amounts and then with `20%` too few `A` groups, and follows the sol and gel fractions through six conversions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace step_growth = chemistry::step_growth;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a network polymer from a three-armed monomer (functionality 3) and a two-armed one (functionality 2), mixed in equal amounts
  const double f = 3.0, g = 2.0;
  const auto ratio = vec({1.0});
  out.writeln("conversion at the gel point (equal amounts)  = ", flat(step_growth::critical_conversion(f, g, ratio)));
  out.writeln("conversion at the gel point (r = 0.8)        = ", flat(step_growth::critical_conversion(f, g, vec({0.8}))));

  // how much of the sample is still free (soluble) as the reaction proceeds
  const auto conversion = vec({0.5, 0.7, 0.75, 0.8, 0.9, 1.0});
  const auto gel = step_growth::sol_gel(conversion, ratio.expand({6}), f, g);
  out.writeln("conversion of A groups   ", flat(conversion));
  out.writeln("weight fraction soluble  ", flat(gel.sol_fraction));
  out.writeln("weight fraction in gel   ", flat(gel.gel_fraction));
}
```

Output

```text
conversion at the gel point (equal amounts)  = [0.7071]
conversion at the gel point (r = 0.8)        = [0.7906]
conversion of A groups   [0.5000, 0.7000, 0.7500, 0.8000, 0.9000, 1.0000]
weight fraction soluble  [1.0000, 1.0000, 0.4853, 0.1944, 0.0186, 0.0000]
weight fraction in gel   [0.0000, 0.0000, 0.5147, 0.8056, 0.9814, 1.0000]
```

With equal amounts, the gel point is at a conversion of `0.7071`, which is `1/sqrt(2)`; with `r = 0.8` it moves to `0.7906`, because with fewer `A` groups the reaction must go further before a network can span the sample. At conversions `0.5` and `0.7`, below the gel point, the whole sample is sol. The gel then appears abruptly: at conversion `0.75` already `0.5147` of the sample is gel, `0.8056` at `0.8`, and by full conversion nothing is left in solution.

<!-- addex:start -->
## Rate of conversion

For a second-order reaction the conversion grows with time, {math}`p = kc_0t/(1 + kc_0t)`, and the degree of polymerization is {math}`1/(1-p)`. `step_growth::conversion_second_order(t, k, c0)` evaluates it, and `conversion_imbalanced(t, k, ca0, r)` handles an excess of one group, with {math}`r < 1` the ratio of the deficient to the plentiful group. The program evaluates both at four times with {math}`k = 0.01` and {math}`c_0 = 1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace step_growth = chemistry::step_growth;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // second-order step growth with rate constant k and initial concentration c0: conversion against time, and the degree of polymerization 1/(1-p)
  const auto t = vec({1.0, 10.0, 100.0, 1000.0});
  const auto p = step_growth::conversion_second_order(t, 0.01, 1.0);
  out.writeln("time                          ", flat(t));
  out.writeln("conversion p = k c0 t/(1+k c0 t) ", flat(p));
  out.writeln("degree of polymerization 1/(1-p) ", flat(p.neg().add(1.0).reciprocal()));
  // an imbalance of the two groups limits the conversion: r = 0.9, i.e. 10 percent too few A groups
  out.writeln("conversion with imbalance r = 0.9 ", flat(step_growth::conversion_imbalanced(t, 0.01, 1.0, 0.9)));
}
```

Output

```text
time                          [1.0000, 10.0000, 100.0000, 1000.0000]
conversion p = k c0 t/(1+k c0 t) [0.0099, 0.0909, 0.5000, 0.9091]
degree of polymerization 1/(1-p) [1.0100, 1.1000, 2.0000, 11.0000]
conversion with imbalance r = 0.9 [0.0099, 0.0913, 0.5126, 0.9450]
```

Conversion is slow, `0.0909` at time `10` and `0.5000` at `100`, and the chains stay short, `2.0000` at half conversion and only `11.0000` at conversion `0.9091`: high molar mass needs very high conversion. With a `10%` imbalance the conversion at the same times is `0.0099`, `0.0913`, `0.5126` and `0.9450`, a little higher than the balanced values at the later times.

## Branching before the gel point

For a monomer with {math}`f` reactive groups the average size grows without limit as the gel point is approached. `afb_critical_conversion(f, ...)` gives the gel point, and `afb_number_average(p, f)` and `afb_weight_average(p, f)` the average sizes before it. The program evaluates them at `50%`, `80%` and `95%` of the gel point for three functionalities.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace step_growth = chemistry::step_growth;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // branched products from a monomer with f = 3, 4, 6 reactive groups: the gel point and the average sizes just before it
  // the averages are valid up to the gel point: evaluate at 50%, 80% and 95% of it
  for (double f : {3.0, 4.0, 6.0}) {
    const double gel = flat(step_growth::afb_critical_conversion(f, vec({0.0})))[0];
    const auto p = vec({0.5 * gel, 0.8 * gel, 0.95 * gel});
    out.writeln("functionality ", f, ": gel point at p = ", gel);
    out.writeln("    conversion               ", flat(p));
    out.writeln("    number-average size      ", flat(step_growth::afb_number_average(p, f)));
    out.writeln("    weight-average size      ", flat(step_growth::afb_weight_average(p, f)));
  }
}
```

Output

```text
functionality 3.0000: gel point at p = 0.5000
    conversion               [0.2500, 0.4000, 0.4750]
    number-average size      [2.0000, 5.0000, 20.0000]
    weight-average size      [3.5000, 17.0000, 219.5000]
functionality 4.0000: gel point at p = 0.3333
    conversion               [0.1667, 0.2667, 0.3167]
    number-average size      [2.0000, 5.0000, 20.0000]
    weight-average size      [3.6667, 19.6667, 279.6667]
functionality 6.0000: gel point at p = 0.2000
    conversion               [0.1000, 0.1600, 0.1900]
    number-average size      [2.0000, 5.0000, 20.0000]
    weight-average size      [3.8000, 21.8000, 327.8000]
```

A larger functionality gels sooner, at conversions `0.5000`, `0.3333` and `0.2000` for {math}`f = 3, 4, 6`. Measured as a fraction of the gel point the number-average size is the same for all, `2.0000`, `5.0000` and `20.0000`, but the weight average diverges much faster, to `219.5` for {math}`f = 3` and `327.8` for {math}`f = 6` at `95%` of the gel point: a few very large branched molecules dominate the mass just before gelation.

<!-- addex:end -->

## Decisions

- Use `critical_conversion(f, g, r)` for a mixture of two monomers with functionalities `f` and `g` and a stoichiometric ratio `r`; use `afb_critical_conversion` when one branched monomer reacts with itself.
- Ask `sol_gel` for the soluble and gelled fractions once you are past the gel point; the number- and weight-average functions describe only the finite chains before it.
- Add a deliberate imbalance (`conversion_imbalanced`) if you want to cap the conversion and so the chain length.

## Pitfalls

The average-size formulas for branched monomers hold only up to the gel point. The program evaluates the weight-average size for a three-functional monomer on both sides of it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;

namespace step_growth = chemistry::step_growth;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the branched-product averages hold only up to the gel point; past it they no longer describe a finite size
  const double f = 3.0;
  const double gel = flat(step_growth::afb_critical_conversion(f, vec({0.0})))[0];
  const auto p = vec({0.9 * gel, 0.99 * gel, 1.01 * gel, 1.2 * gel});
  out.writeln("gel point ", gel);
  out.writeln("conversion            ", flat(p));
  out.writeln("weight-average size   ", flat(step_growth::afb_weight_average(p, f)));
}
```

Output

```text
gel point 0.5000
conversion            [0.4500, 0.4950, 0.5050, 0.6000]
weight-average size   [59.5000, 5099.5000, 4899.5000, 7.0000]
```

The gel point is `0.5000`. The weight-average size grows from `59.5000` at 90 percent of it to `5099.5000` at 99 percent. Just past the gel point and further it returns `4899.5000` and `7.0000`, numbers with no meaning for a sample that now holds an infinite network. Past the gel point, ask `sol_gel` for the soluble fraction instead.

## Where next

[Nucleation and growth](../chem_03_crystallization/nucleation_and_growth.md) follows another transition, from a solution to a solid. Return to [Chain statistics](chain_statistics.md) for the averages that apply before gelation.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="chain_statistics.html">
    <span class="label">Previous</span>
    <span class="title">Chain statistics</span>
  </a>
</div>
<!-- apiref:nav:end -->
