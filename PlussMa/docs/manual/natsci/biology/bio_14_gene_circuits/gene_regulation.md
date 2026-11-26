# Gene regulation

[Back to Biology 14: Gene Circuits](../bio_14_gene_circuits.md)

Genes switch each other on and off through the proteins they make, and a few simple wiring patterns produce memory and clocks. This page simulates three classic circuits, a toggle switch that remembers its state, a ring of three repressors that oscillates, and a gene that represses itself through a delay, and shows how the dynamics depend on how steeply a gene responds to its repressor.


## How it works

Each circuit is a set of rate equations, and each `gene_networks::*_rate` function returns the right-hand side {math}`\mathrm{d}x/\mathrm{d}t` for a state with the species on its last axis, so it can be handed straight to `dynamics::simulate(rate, start, t0, t1, steps)`. The start may have a leading batch axis, so a whole set of cells is integrated in one call. Three things decide what a circuit does: how many genes repress each other (two give memory, an odd ring gives a clock), how steeply a gene responds to its repressor (the Hill coefficient), and how long the chain of delays is. The programs below take each in turn.

## A toggle switch

In the genetic toggle switch two genes repress each other. With expression levels {math}`u` and {math}`v` and repression of Hill strength {math}`\beta` and {math}`\gamma`,

```{math}
\begin{aligned}
&\dot u = \frac{a_1}{1 + v^{\beta}} - u, \\
&\dot v = \frac{a_2}{1 + u^{\gamma}} - v
\end{aligned}
```

`gene_networks::toggle_switch_rate(s, a1, a2, beta, gamma)` is the right-hand side for the state `(u, v)`, with all the coefficients allowed to be arrays. With strong enough repression the circuit has two stable states, one gene on and the other off, and remembers which one it was pushed into. The program starts five cells at different expression levels with {math}`a_1 = a_2 = 5` and {math}`\beta = \gamma = 2` and runs them together to time `60`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace systems = biology::systems;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace gene = systems::gene_networks;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a genetic toggle switch: two genes that repress each other. Start from five different expression levels and see where each ends up
  const auto start = vec({0.1, 3.0, 2.0, 2.0, 3.0, 0.1, 1.0, 1.1, 1.1, 1.0}).reshape({5, 2});
  const auto rate = [](const pluss::Series &s) { return gene::toggle_switch_rate(s, 5.0, 5.0, 2.0, 2.0); };
  const auto end = dynamics::simulate(rate, start, 0.0, 60.0, 3000);
  out.writeln("start (u, v) per row = ", flat(start));
  out.writeln("end   (u, v) per row = ", flat(end));
}
```

Output

```text
start (u, v) per row = [0.1000, 3.0000, 2.0000, 2.0000, 3.0000, 0.1000, 1.0000, 1.1000, 1.1000, 1.0000]
end   (u, v) per row = [0.2087, 4.7913, 1.5160, 1.5160, 4.7913, 0.2087, 0.2087, 4.7913, 4.7913, 0.2087]
```

In the toggle switch the cells that start with {math}`u` low and {math}`v` high end at `(0.2087, 4.7913)`, and those that start the opposite way end at `(4.7913, 0.2087)`: two stable states, each reached from the side the cell started on. The nearly balanced cells `(1.0, 1.1)` and `(1.1, 1.0)` are pushed to the state of the gene that has the small lead. The cell started exactly on the diagonal, `(2.0, 2.0)`, stays there at `(1.5160, 1.5160)` by symmetry, an unstable state that any asymmetry would break.

## A ring of three repressors

In the repressilator each gene represses the next in a ring of three. `gene_networks::repressilator_rate(s, alpha, alpha0, beta, n)` is the rate for the state of three mRNAs followed by three proteins, with the maximum transcription `alpha`, the leak `alpha0`, the ratio of protein to mRNA decay `beta` and the Hill coefficient `n`. Because the repression goes round a ring with an odd number of inhibitions, the circuit has no stable steady state and oscillates. The program lets the circuit settle for `100` time units, then records the first protein for `60` more and finds its peaks.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace systems = biology::systems;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace gene = systems::gene_networks;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // the repressilator: three genes in a ring of repression. State (mRNA 1..3, protein 1..3); the first protein's range over time
  const auto rate = [](const pluss::Series &s) { return gene::repressilator_rate(s, 216.0, 0.216, 5.0, 2.0); };
  auto s = vec({0.0, 0.0, 0.0, 5.0, 0.0, 15.0});
  s = dynamics::simulate(rate, s, 0.0, 100.0, 10000);          // pass the transient
  double lo = 1e9, hi = -1e9; std::vector<double> peaks;
  double previous = 0.0, before = 0.0, t = 0.0;
  for (int k = 0; k < 600; ++k) {
    s = dynamics::simulate(rate, s, 0.0, 0.1, 20); t += 0.1;
    const double x = flat(s)[3];
    lo = std::min(lo, x); hi = std::max(hi, x);
    if (k >= 2 && previous > before && previous > x) peaks.push_back(t - 0.1);
    before = previous; previous = x;
  }
  out.writeln("protein 1 oscillates between ", lo, " and ", hi);
  out.writeln("times of its peaks: ", peaks);
  if (peaks.size() > 1) out.writeln("period = ", (peaks.back() - peaks.front()) / (peaks.size() - 1));
}
```

Output

```text
protein 1 oscillates between 1.250 and 59.212
times of its peaks: [1.900, 10.500, 19.200, 27.900, 36.600, 45.300, 54.000]
period = 8.683
```

The first protein oscillates between `1.250` and `59.212`, with peaks at `1.900`, `10.500`, `19.200` and on, a period of `8.683` time units. There is no stable steady state: the ring of three inhibitions keeps chasing itself.

## How steep must the repression be?

The Goodwin oscillator is a single gene that represses itself through two further steps, {math}`\dot x = a/(1 + z^n) - bx`, {math}`\dot y = cx - by`, {math}`\dot z = cy - bz`, `gene_networks::goodwin_rate(s, a, b, c, n)`. The steady state loses stability only when the Hill coefficient {math}`n` is large enough. The program settles three circuits and prints the range of the last protein over `300` further time units.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace systems = biology::systems;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace gene = systems::gene_networks;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the Goodwin oscillator: a protein represses its own gene through a chain of two delays. Oscillations need a steep enough repression (Hill coefficient n)
  for (double n : {4.0, 8.0, 12.0}) {
    const auto rate = [&](const pluss::Series &s) { return gene::goodwin_rate(s, 1.0, 0.1, 0.1, n); };
    auto s = vec({0.5, 0.5, 0.5});
    s = dynamics::simulate(rate, s, 0.0, 600.0, 12000);
    double lo = 1e9, hi = -1e9;
    for (int k = 0; k < 300; ++k) { s = dynamics::simulate(rate, s, 0.0, 1.0, 20); const double z = flat(s)[2]; lo = std::min(lo, z); hi = std::max(hi, z); }
    out.writeln("Hill coefficient ", n, ": the repressor settles to the range [", lo, ", ", hi, "]");
  }
}
```

Output

```text
Hill coefficient 4.0000: the repressor settles to the range [1.5330, 1.5330]
Hill coefficient 8.0000: the repressor settles to the range [1.2558, 1.2908]
Hill coefficient 12.0000: the repressor settles to the range [0.9979, 1.5856]
```

At a Hill coefficient of `4` the repressor settles to the single value `1.5330`: no oscillation. At `8` it swings only between `1.2558` and `1.2908`, and at `12` between `0.9979` and `1.5856`. The oscillation appears, and grows, as the repression gets steeper.

## Decisions

- Use a toggle switch when you want a circuit that stores one bit: two stable states that a brief signal can flip.
- Use the repressilator or the Goodwin loop when you want a clock; the Goodwin loop needs a steep repression (large Hill coefficient) and the repressilator needs three genes in a ring.
- Integrate the transient away before you measure an oscillation, and record the amplitude and the period over a window longer than one cycle.

## Pitfalls

A circuit's state has a fixed length, a symmetric start can sit on an unstable state, and a coarse step count changes the answer. The program tries all three.

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
namespace gene = systems::gene_networks;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the repressilator state is three mRNAs followed by three proteins: six numbers, not three
  const auto rate = [](const pluss::Series &s) { return gene::repressilator_rate(s, 216.0, 0.216, 5.0, 2.0); };
  try { (void)dynamics::simulate(rate, vec({0.0, 0.0, 0.0}), 0.0, 1.0, 100); out.writeln("three numbers accepted"); }
  catch (const pluss::Error &e) { out.writeln("three numbers: ", e.message()); }
  // a toggle switch started exactly on the diagonal stays there, even though that state is unstable
  const auto toggle = [](const pluss::Series &s) { return gene::toggle_switch_rate(s, 5.0, 5.0, 2.0, 2.0); };
  out.writeln("start (2, 2):            ", flat(dynamics::simulate(toggle, vec({2.0, 2.0}), 0.0, 60.0, 3000)));
  out.writeln("start (2, 2.000001):     ", flat(dynamics::simulate(toggle, vec({2.0, 2.000001}), 0.0, 60.0, 3000)));
  // with steps that are too coarse the integration is not trustworthy; compare two step counts
  const auto fast = [](const pluss::Series &s) { return gene::goodwin_rate(s, 1.0, 0.1, 0.1, 12.0); };
  out.writeln("Goodwin, n = 12, 300 steps:   ", flat(dynamics::simulate(fast, vec({0.5, 0.5, 0.5}), 0.0, 600.0, 300)));
  out.writeln("Goodwin, n = 12, 12000 steps: ", flat(dynamics::simulate(fast, vec({0.5, 0.5, 0.5}), 0.0, 600.0, 12000)));
}
```

Output

```text
three numbers: natsci: repressilator: state must have 6 species on its last axis
start (2, 2):            [1.5160, 1.5160]
start (2, 2.000001):     [0.2087, 4.7913]
Goodwin, n = 12, 300 steps:   [2.7187, 1.3999, 1.0542]
Goodwin, n = 12, 12000 steps: [2.6968, 1.3618, 1.0442]
```

Three numbers for the repressilator are rejected: it needs six, three mRNAs and three proteins. The toggle switch started at `(2, 2)` stays on the unstable diagonal at `(1.5160, 1.5160)`, but at `(2, 2.000001)` it falls to `(0.2087, 4.7913)`, so use a slightly asymmetric start when you want to see where a real cell would go. For the steep Goodwin loop `300` steps and `12000` steps give `(2.7187, 1.3999, 1.0542)` and `(2.6968, 1.3618, 1.0442)` at time `600`, so refine the step count until the answer stops changing before you trust a phase.

## Where next

[Biology 21: Delays](../bio_21_delays.md) shows how a delay alone can produce the oscillations this page gets from steep repression, and [Biology 20: Metabolic Control](../bio_20_control.md) asks how strongly a pathway's flux responds to its parameters.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
