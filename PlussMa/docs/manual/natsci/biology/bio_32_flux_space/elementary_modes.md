# Elementary modes

[Back to Biology 32: The Flux Space](../bio_32_flux_space.md)

You have a metabolic network and you want a list of its pathways: the minimal sets of reactions that can run at steady state, from which every other steady-state flux is built. This page enumerates the elementary flux modes of a network, reads them, and shows how a reversible reaction appears in them.

## How it works

A flux vector $v$ is at steady state when $S v = 0$ with irreversible reactions non-negative. An *elementary mode* is a steady-state flux vector whose set of active reactions (its support) is minimal: no other steady-state vector uses only a proper subset of its reactions. Every steady-state flux is a non-negative combination of elementary modes, plus the free directions of any reversible reactions, so the modes are the pathways of the network. `metabolism::elementary_modes(net)` returns a Series `[modes, reactions]`, each row scaled to a largest absolute entry of one. A reversible reaction (one whose lower bound is negative) is split internally into a forward and a backward step, and the result shows it once with its natural sign. The enumeration is combinatorial and runs on the host, so use it for networks of up to a few hundred reactions; there is a cap `max_modes` (two million by default) on the number of modes. Bounds other than the sign of the lower bound do not matter.

## The pathways of a bypass

The program lists the elementary modes of the bypass network and checks that each is a steady state.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto net = bypass();
  // elementary modes: the minimal pathways of the network, each a flux vector scaled to a largest entry of 1
  const auto modes = metabolism::elementary_modes(net);
  out.writeln("number of modes ", std::vector<double>{static_cast<double>(modes.size(0))});
  for (std::int64_t i = 0; i < modes.size(0); ++i)
    out.writeln("mode ", std::vector<double>{static_cast<double>(i + 1)}, " (uptake, A->B, A->C, C->B, biomass) ", flat(modes.select(0, i)));
  // each mode satisfies the steady state S v = 0
  out.writeln("largest |S v| over the modes ", std::vector<double>{net.stoichiometry.matmul(modes.transpose(0, 1)).abs().max().item<double>()});
}
```

Output

```text
number of modes [2.0000]
mode [1.0000] (uptake, A->B, A->C, C->B, biomass) [1.0000, 1.0000, 0.0000, 0.0000, 1.0000]
mode [2.0000] (uptake, A->B, A->C, C->B, biomass) [1.0000, 0.0000, 1.0000, 1.0000, 1.0000]
largest |S v| over the modes [0.0000]
```

The network has two modes. In the first, uptake goes directly through $A\to B$ to biomass, `(1, 1, 0, 0, 1)`; in the second it goes through $A\to C\to B$, `(1, 0, 1, 1, 1)`. These are the two pathways. Every flux the network can carry is a non-negative combination of them, with their weights fixed by the bounds, and the largest entry of $S v$ over the two modes is `0.0000`.

## A reversible reaction

The program defines a small network with a reversible conversion $A\leftrightarrow B$, two uptake reactions and two exports, and lists its modes.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a network with a reversible step: A <-> B can run either way. Reactions: 0 A uptake, 1 A <-> B (reversible), 2 B -> export, 3 A -> export, 4 B uptake
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, 0.0, -1.0, 0.0}, {0.0, 1.0, -1.0, 0.0, 1.0}});
  net.lower = vec({0.0, -100.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 100.0, 100.0, 100.0, 10.0});
  const auto modes = metabolism::elementary_modes(net);
  out.writeln("modes ", std::vector<double>{static_cast<double>(modes.size(0))});
  for (std::int64_t i = 0; i < modes.size(0); ++i)
    out.writeln("mode ", std::vector<double>{static_cast<double>(i + 1)}, " ", flat(modes.select(0, i)));
  // the reversible reaction appears once with its natural sign: positive means A -> B, negative B -> A
  const auto bypass_modes = metabolism::elementary_modes(bypass());
  out.writeln("for comparison, the bypass network has ", std::vector<double>{static_cast<double>(bypass_modes.size(0))}, " modes");
}
```

Output

```text
modes [4.0000]
mode [1.0000] [1.0000, 0.0000, 0.0000, 1.0000, 0.0000]
mode [2.0000] [0.0000, 0.0000, 1.0000, 0.0000, 1.0000]
mode [3.0000] [1.0000, 1.0000, 1.0000, 0.0000, 0.0000]
mode [4.0000] [0.0000, -1.0000, 0.0000, 1.0000, 1.0000]
for comparison, the bypass network has [2.0000] modes
```

There are four modes. A can be exported without ever being converted, `(1, 0, 0, 1, 0)`; A can be converted and B exported, `(1, 1, 1, 0, 0)`; B taken up can be exported directly, `(0, 0, 1, 0, 1)`; and B taken up can be converted into A and exported, `(0, -1, 0, 1, 1)`, where the negative entry means the reversible step runs from B to A. The reversible reaction appears once, with the sign of the direction in which it is used.

## Counting pathways

The program counts the modes of the bypass network and the length of each, then adds a second enzyme for $A\to B$ as a sixth reaction and lists the modes again.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto first = metabolism::elementary_modes(bypass());
  out.writeln("first network: modes ", std::vector<double>{static_cast<double>(first.size(0))});
  for (std::int64_t i = 0; i < first.size(0); ++i)
    out.writeln("   mode ", std::vector<double>{static_cast<double>(i + 1)}, " uses ", std::vector<double>{first.select(0, i).ne(0.0).to(pluss::ScalarType::Float64).sum().item<double>()}, " reactions");
  // add a third route A -> B through a second enzyme: one more mode, and every extra parallel route adds one
  metabolism::Network net = bypass();
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0, -1.0}, {0.0, 1.0, 0.0, 1.0, -1.0, 1.0}, {0.0, 0.0, 1.0, -1.0, 0.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0, 6.0});
  const auto three = metabolism::elementary_modes(net);
  out.writeln("with a second A -> B enzyme: modes ", std::vector<double>{static_cast<double>(three.size(0))});
  for (std::int64_t i = 0; i < three.size(0); ++i)
    out.writeln("   mode ", std::vector<double>{static_cast<double>(i + 1)}, " ", flat(three.select(0, i)));
}
```

Output

```text
first network: modes [2.0000]
   mode [1.0000] uses [3.0000] reactions
   mode [2.0000] uses [4.0000] reactions
with a second A -> B enzyme: modes [3.0000]
   mode [1.0000] [1.0000, 1.0000, 0.0000, 0.0000, 1.0000, 0.0000]
   mode [2.0000] [1.0000, 0.0000, 0.0000, 0.0000, 1.0000, 1.0000]
   mode [3.0000] [1.0000, 0.0000, 1.0000, 1.0000, 1.0000, 0.0000]
```

The bypass network has two modes, one using `3` reactions and one using `4`: the pathway through C is longer. A second enzyme for $A\to B$ adds exactly one mode, `(1, 0, 0, 0, 1, 1)`, in which it replaces the first enzyme, so there are now `3` modes. Counting modes measures the redundancy of a network: each parallel route adds a way to make biomass.

## Decisions

- Use elementary modes for small and medium networks when you want the complete list of pathways; for a genome-scale model use flux sampling or flux variability instead, because the number of modes grows combinatorially.
- Mark reversibility through a negative lower bound: nothing else about the bounds changes the modes.
- Check the result with $S v = 0$ when you build a network by hand: a mode that is not a steady state shows a mistake in the stoichiometry.
- Raise `max_modes` only if you are sure the enumeration will finish.

## Pitfalls

A mode is a pathway, not a flux you can observe. The program shows that two pathways can run at once with any weights, and that the weights are limited by the bounds.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace metabolism = biology::metabolism;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// metabolites A, B, C; reactions: 0 uptake -> A, 1 A -> B, 2 A -> C, 3 C -> B, 4 B -> biomass
static metabolism::Network bypass() {
  metabolism::Network net;
  net.stoichiometry = mat({{1.0, -1.0, -1.0, 0.0, 0.0}, {0.0, 1.0, 0.0, 1.0, -1.0}, {0.0, 0.0, 1.0, -1.0, 0.0}});
  net.lower = vec({0.0, 0.0, 0.0, 0.0, 0.0});
  net.upper = vec({10.0, 6.0, 100.0, 8.0, 100.0});
  return net;
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto net = bypass();
  const auto modes = metabolism::elementary_modes(net);
  // a flux built from the two modes with weights 4 and 6: it is a steady state and inside the bounds
  const auto v = modes.select(0, 0).mul(4.0).add(modes.select(0, 1).mul(6.0));
  out.writeln("4 x mode 1 + 6 x mode 2 ", flat(v));
  out.writeln("steady state (S v)      ", flat(net.stoichiometry.matmul(v.unsqueeze(-1))));
  out.writeln("within the bounds       ", std::vector<double>{v.le(net.upper).all().item<bool>() && v.ge(net.lower).all().item<bool>() ? 1.0 : 0.0});
  // weights 7 and 3: a steady state, but the direct route A -> B would carry 7, above its bound of 6
  const auto w = modes.select(0, 0).mul(7.0).add(modes.select(0, 1).mul(3.0));
  out.writeln("7 x mode 1 + 3 x mode 2 ", flat(w));
  out.writeln("steady state (S v)      ", flat(net.stoichiometry.matmul(w.unsqueeze(-1))));
  out.writeln("within the bounds       ", std::vector<double>{w.le(net.upper).all().item<bool>() && w.ge(net.lower).all().item<bool>() ? 1.0 : 0.0});
}
```

Output

```text
4 x mode 1 + 6 x mode 2 [10.0000, 4.0000, 6.0000, 6.0000, 10.0000]
steady state (S v)      [0.0000, 0.0000, 0.0000]
within the bounds       [1.0000]
7 x mode 1 + 3 x mode 2 [10.0000, 7.0000, 3.0000, 3.0000, 10.0000]
steady state (S v)      [0.0000, 0.0000, 0.0000]
within the bounds       [0.0000]
```

The flux vector `(10, 4, 6, 6, 10)` is the sum of `4` times the first mode and `6` times the second, and it respects the bounds of the network, but the combination with weights `(7, 3)` would need `7` units through the direct route, whose upper bound is `6`. The modes say which pathways exist; the bounds say how much each can carry.

## Where next

Continue with [Flux sampling](flux_sampling.md) to see the whole polytope of allowed fluxes, and with [Biology 4: Metabolism](../bio_04_metabolism.md) for flux balance and variability.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="flux_sampling.html">
    <span class="label">Next</span>
    <span class="title">Flux sampling</span>
  </a>
</div>
<!-- apiref:nav:end -->
