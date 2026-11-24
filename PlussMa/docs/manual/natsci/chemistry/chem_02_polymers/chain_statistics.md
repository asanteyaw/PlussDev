# Chain statistics

[Back to Chemistry 2: Polymers](../chem_02_polymers.md)

A polymer sample is a mixture of chains of different lengths, so one number cannot describe its mass. This page computes the number- and weight-average molar masses of a sample and their ratio, the Carothers relation for chains grown by step-growth, and the composition of a copolymer made from two monomers that prefer different partners.

## How it works

`polymers::molecular_weights(length, count, m0)` takes the length `x` of each kind of chain in monomer units and how many chains there are of each, and returns `mn`, the number-average molar mass; `mw`, the weight-average, which counts long chains more; and `dispersity = mw / mn`, which is `1` only when every chain has the same length. `carothers_degree_of_polymerization(p)` is `1 / (1 - p)`: the average chain length when a fraction `p` of the reactive groups has reacted. `mayo_lewis(f1, r1, r2)` is the fraction of monomer `1` in the copolymer formed from a feed with fraction `f1` of that monomer, when `r1` and `r2` measure how strongly each growing chain end prefers its own monomer.

## Molar mass, conversion and copolymer composition

The sample holds `400`, `300`, `200` and `100` chains of lengths `100`, `200`, `400` and `800` monomer units of molar mass `104` g/mol. The program prints the averages, the Carothers lengths at three conversions, and the copolymer composition for `r1 = 0.5` and `r2 = 2` across five feeds.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
namespace polymers = chemistry::polymers;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a sample: chains of length x (in monomer units) and how many there are of each
  const auto length = vec({100.0, 200.0, 400.0, 800.0});
  const auto count = vec({400.0, 300.0, 200.0, 100.0});
  const auto w = polymers::molecular_weights(length, count, 104.0);   // 104 g/mol per styrene unit
  out.writeln("number-average molar mass Mn = ", flat(w.mn));
  out.writeln("weight-average molar mass Mw = ", flat(w.mw));
  out.writeln("dispersity Mw/Mn             = ", flat(w.dispersity));

  // the chains from an ideal step-growth reaction at conversion p have the most probable distribution
  const auto p = vec({0.5, 0.9, 0.99});
  out.writeln("conversion p                  ", flat(p));
  out.writeln("Carothers degree of polymerization 1/(1-p) = ", flat(polymers::carothers_degree_of_polymerization(p)));

  // copolymerization: two monomers with reactivity ratios r1 = 0.5, r2 = 2
  const auto feed = vec({0.1, 0.3, 0.5, 0.7, 0.9});
  out.writeln("monomer 1 in the feed          ", flat(feed));
  out.writeln("monomer 1 in the copolymer     ", flat(polymers::mayo_lewis(feed, 0.5, 2.0)));
}
```

Output

```text
number-average molar mass Mn = [27040.0000]
weight-average molar mass Mw = [44800.0000]
dispersity Mw/Mn             = [1.6568]
conversion p                  [0.5000, 0.9000, 0.9900]
Carothers degree of polymerization 1/(1-p) = [2.0000, 10.0000, 100.0000]
monomer 1 in the feed          [0.1000, 0.3000, 0.5000, 0.7000, 0.9000]
monomer 1 in the copolymer     [0.0526, 0.1765, 0.3333, 0.5385, 0.8182]
```

The number-average molar mass is `27040` and the weight-average `44800`, so the dispersity is `1.6568`: a few long chains carry much of the mass. The Carothers relation shows how demanding step-growth is: conversion `0.5` gives chains of only `2` units, `0.9` gives `10` and `0.99` gives `100`. In the copolymer, monomer `1` is under-represented relative to the feed at every composition, `0.1765` in the polymer from a feed of `0.3`, because chains ending in monomer `2` strongly prefer to add another monomer `2`.

<!-- addex:start -->
## How big is one chain?

A chain in solution is a random coil. For a freely jointed chain of {math}`N` bonds of length {math}`b` the mean square end-to-end distance is {math}`Nb^2`, `polymers::fjc_mean_square_end_to_end(N, b)`. A stiffer chain, the worm-like chain of contour length {math}`L` and persistence length {math}`\ell_p`, `polymers::wlc_mean_square_end_to_end(L, lp)`, is straight on scales below {math}`\ell_p` and coiled beyond it. The program evaluates both.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace polymers = chemistry::polymers;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // how big is one chain? mean square end-to-end distance (nm^2) of a freely jointed chain of N bonds of 0.15 nm, and of a worm-like chain of contour length 1 um
  const auto n = vec({10.0, 100.0, 1000.0, 10000.0});
  out.writeln("number of bonds                       ", flat(n));
  out.writeln("freely jointed chain, <R^2> (nm^2)    ", flat(polymers::fjc_mean_square_end_to_end(n, 0.15)));
  const auto contour = vec({10.0, 50.0, 100.0, 1000.0});
  out.writeln("contour length (nm)                   ", flat(contour));
  out.writeln("worm-like chain, persistence 50 nm    ", flat(polymers::wlc_mean_square_end_to_end(contour, 50.0)));
}
```

Output

```text
number of bonds                       [10.0000, 100.0000, 1000.0000, 10000.0000]
freely jointed chain, <R^2> (nm^2)    [0.2250, 2.2500, 22.5000, 225.0000]
contour length (nm)                   [10.0000, 50.0000, 100.0000, 1000.0000]
worm-like chain, persistence 50 nm    [93.6538, 1839.3972, 5676.6764, 95000.0000]
```

The freely jointed chain grows in proportion to the number of bonds, `0.2250` nm² for `10` bonds to `225.0000` for `10000`. The worm-like chain behaves like a rod when it is shorter than the persistence length, `93.6538` nm² for a contour of `10` nm (close to `10² = 100`), and like a coil when it is much longer, `95000` nm² for `1000` nm, which is nearly {math}`2L\ell_p = 100000`.

## The most probable distribution

Chains made by an ideal step-growth reaction follow the Flory distribution: at conversion {math}`p` the number fraction of chains of length {math}`x` is {math}`(1-p)p^{x-1}` and the weight fraction {math}`x(1-p)^2p^{x-1}`. `flory_number_fraction(x, p)` and `flory_weight_fraction(x, p)` evaluate them. The program prints both at three conversions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace polymers = chemistry::polymers;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // the most probable (Flory) distribution of chain lengths at conversions 0.5, 0.8 and 0.95: number fraction of x-mers
  const auto x = vec({1.0, 2.0, 5.0, 10.0, 20.0, 50.0});
  out.writeln("chain length x                    ", flat(x));
  for (double p : {0.5, 0.8, 0.95}) {
    out.writeln("number fraction at p = ", p, "     ", flat(polymers::flory_number_fraction(x, p)));
    out.writeln("weight fraction at p = ", p, "     ", flat(polymers::flory_weight_fraction(x, p)));
  }
}
```

Output

```text
chain length x                    [1.00000, 2.00000, 5.00000, 10.00000, 20.00000, 50.00000]
number fraction at p = 0.50000     [0.50000, 0.25000, 0.03125, 0.00098, 0.00000, 0.00000]
weight fraction at p = 0.50000     [0.25000, 0.25000, 0.07812, 0.00488, 0.00001, 0.00000]
number fraction at p = 0.80000     [0.20000, 0.16000, 0.08192, 0.02684, 0.00288, 0.00000]
weight fraction at p = 0.80000     [0.04000, 0.06400, 0.08192, 0.05369, 0.01153, 0.00004]
number fraction at p = 0.95000     [0.05000, 0.04750, 0.04073, 0.03151, 0.01887, 0.00405]
weight fraction at p = 0.95000     [0.00250, 0.00475, 0.01018, 0.01576, 0.01887, 0.01012]
```

At conversion `0.5` almost all chains are short, with half of them monomers (`0.50000`) and a quarter dimers. At `0.95` the number fraction falls slowly with the length, `0.03151` for `10`-mers, and the weight fraction peaks at a chain length of about `20` (`0.01887`): most of the mass is in long chains although most of the molecules are short.

<!-- addex:end -->

## Decisions

- Report `mn` and `mw` together with the dispersity: they answer different questions (the average chain you would count, and the average chain a given unit of mass belongs to).
- Use `carothers_degree_of_polymerization` for step-growth chains, where the length is set by the conversion `p` of the reactive groups; a high-molar-mass polymer needs `p` very close to 1.
- Use the freely jointed chain for a flexible chain much longer than its stiff segment and `wlc_mean_square_end_to_end` when the stiffness matters.
- Use `mayo_lewis` with reactivity ratios from your own copolymerization to predict composition; a ratio product of 1 gives an ideal random copolymer.

## Pitfalls

The degree of polymerization is a function of the conversion as a *fraction*, and it diverges as `p` approaches 1. The program evaluates it at four conversions near 1, at exactly 1, and at `95` typed as a percentage; the last two are refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;

namespace polymers = chemistry::polymers;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the degree of polymerization 1/(1 - p) needs the conversion p as a fraction, and it grows without bound as p approaches 1
  out.writeln("p = 0.9, 0.99, 0.999, 0.9999    ", flat(polymers::carothers_degree_of_polymerization(vec({0.9, 0.99, 0.999, 0.9999}))));
  for (double p : {1.0, 95.0}) {
    try {
      out.writeln("p = ", p, "   ", flat(polymers::carothers_degree_of_polymerization(vec({p}))));
    } catch (const pluss::Error &e) {
      out.writeln("p = ", p, ": ", e.message());
    }
  }
}
```

Output

```text
p = 0.9, 0.99, 0.999, 0.9999    [10.0000, 100.0000, 1000.0000, 10000.0000]
p = 1.0000: natsci::polymers::carothers_degree_of_polymerization: the degree of polymerization is unbounded at this conversion (1 + r - 2 r p must stay positive)
p = 95.0000: natsci::polymers::carothers_degree_of_polymerization: the conversion must be a fraction between 0 and 1 (95 percent is 0.95)
```

Each extra `9` in the conversion multiplies the chain length by ten: `10`, `100`, `1000`, `10000`. At exactly `1` the chain would be infinitely long, so the call raises an error rather than returning an infinite number. The percentage `95` is refused with a message that says to pass `0.95`.

`molecular_weights` wants the number of chains of each length. A sample described by mass has to be converted first. The program describes 9 g of chains of length 100 and 1 g of chains of length 1000, with and without that conversion.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace chemistry = pluss::ma::natsci::chemistry;

namespace polymers = chemistry::polymers;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a sample weighed rather than counted: 9 g of chains of length 100 and 1 g of chains of length 1000. molecular_weights wants the NUMBER of chains of each length, which is mass / length (times a constant)
  const auto length = vec({100.0, 1000.0});
  const auto masses = vec({9.0, 1.0});
  const auto counts = masses.div(length);
  const auto right = polymers::molecular_weights(length, counts, 0.104);
  const auto wrong = polymers::molecular_weights(length, masses, 0.104);
  out.writeln("counts = mass / length : Mn ", flat(right.mn), ", Mw ", flat(right.mw), ", dispersity ", flat(right.dispersity));
  out.writeln("masses passed as counts: Mn ", flat(wrong.mn), ", Mw ", flat(wrong.mw), ", dispersity ", flat(wrong.dispersity));
}
```

Output

```text
counts = mass / length : Mn [11.4286], Mw [19.7600], dispersity [1.7290]
masses passed as counts: Mn [19.7600], Mw [59.6632], dispersity [3.0194]
```

Counting properly (`mass / length`) gives `mn = 11.4286` and a dispersity of `1.7290`. Passing the masses as if they were counts overweights the long chains: the dispersity jumps to `3.0194`.

## Where next

[Gelation](gelation.md) takes the same chains past the point where they join into one network. [Adsorption isotherms](../chem_01_surfaces/adsorption_isotherms.md) is the other end of the chapter list, how molecules stick to a surface.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="gelation.html">
    <span class="label">Next</span>
    <span class="title">Gelation</span>
  </a>
</div>
<!-- apiref:nav:end -->
