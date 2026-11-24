# Ligand binding

[Back to Biology 15: Binding](../bio_15_binding.md)

Most of biology starts with a molecule binding to a protein: a hormone to its receptor, an oxygen molecule to hemoglobin, a drug to an enzyme. This page computes how much of a receptor is occupied when the ligand is used up by the binding, when two ligands compete, and when binding at one site changes the binding at the others.


## How it works

Binding is a mass balance. A receptor with total amount {math}`R_T` and a ligand with total {math}`L_T` share what is left over: the amount bound {math}`[RL]` and the free amounts obey {math}`[RL] = R\,L/K_d` with {math}`R = R_T - [RL]` and {math}`L = L_T - [RL]`. `binding::receptor_ligand` solves that quadratic for one ligand, `binding::competitive` solves the coupled balances for several ligands, and `binding::mwc` and `binding::adair` describe a protein with several interacting sites. All of them take arrays, so one call can sweep over ligand totals, constants or proteins.

## When the ligand is used up

The familiar hyperbola {math}`L/(K_d + L)` assumes the ligand is so plentiful that binding does not change its concentration. When the receptor is comparable to the ligand that fails. With total receptor {math}`R_T` and total ligand {math}`L_T` the bound complex is the root of a quadratic,

```{math}
[RL] = \frac{(R_T + L_T + K_d) - \sqrt{(R_T + L_T + K_d)^2 - 4 R_T L_T}}{2}
```

`binding::receptor_ligand(R_T, L_T, K_d)` evaluates it for arrays of totals. The program computes the bound fraction of a unit amount of receptor with {math}`K_d = 1` for six ligand totals and prints the hyperbola next to it.

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

namespace binding = systems::binding;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // how much receptor is bound when the ligand is not in great excess? The exact quadratic solution, for a receptor total of 1 and Kd = 1, over a range of ligand totals
  const auto ligand = vec({0.1, 0.5, 1.0, 2.0, 5.0, 20.0});
  const auto bound = systems::binding::receptor_ligand(vec({1.0, 1.0, 1.0, 1.0, 1.0, 1.0}), ligand, 1.0);
  out.writeln("total ligand                  ", flat(ligand));
  out.writeln("fraction of receptor bound    ", flat(bound));
  // the simple hyperbola L / (Kd + L) overestimates binding because it ignores the ligand used up
  out.writeln("hyperbola ignoring depletion  ", flat(ligand.div(ligand.add(1.0))));
}
```

Output

```text
total ligand                  [0.1000, 0.5000, 1.0000, 2.0000, 5.0000, 20.0000]
fraction of receptor bound    [0.0488, 0.2192, 0.3820, 0.5858, 0.8074, 0.9501]
hyperbola ignoring depletion  [0.0909, 0.3333, 0.5000, 0.6667, 0.8333, 0.9524]
```

With {math}`K_d = 1` and one unit of receptor the fraction bound is `0.0488` at a ligand total of `0.1`, against `0.0909` for the hyperbola: the hyperbola overestimates the binding by almost a factor of two because it ignores the ligand the receptor has taken. The two agree only when the ligand is in large excess (`0.9501` and `0.9524` at `20`).

## Two ligands competing

With several ligands for one receptor, each is bound at the expense of the others. `binding::competitive(R_T, L_T, K_d)` solves all the mass-balance equations together and returns, for each ligand, the `occupancy` (the fraction of receptor that carries it), the free receptor and the free ligands. All the totals must be positive. The program fixes ligand `1` at `2` with {math}`K_d = 1` and adds ligand `2` with {math}`K_d = 10` at three levels.

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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // two ligands compete for one receptor, with Kd 1 and 10: total receptor 1, ligand 1 at 2 and ligand 2 at 0.5, 5 and 20 (every total must be positive)
  for (double second : {0.5, 5.0, 20.0}) {
    const auto r = systems::binding::competitive(vec({1.0}), vec({2.0, second}).reshape({1, 2}), vec({1.0, 10.0}));
    out.writeln("second ligand total ", second, ": occupancy by each = ", flat(r.occupancy), ", free receptor = ", flat(r.free_receptor), ", free ligands = ", flat(r.free_ligand));
  }
}
```

Output

```text
second ligand total 0.5000: occupancy by each = [0.5760, 0.0194], free receptor = [0.4045], free ligands = [1.4240, 0.4806]
second ligand total 5.0000: occupancy by each = [0.5023, 0.1623], free receptor = [0.3354], free ligands = [1.4977, 4.8377]
second ligand total 20.0000: occupancy by each = [0.3571, 0.4255], free receptor = [0.2174], free ligands = [1.6429, 19.5745]
```

The weaker ligand `2` at a total of `0.5` occupies only `0.0194` of the receptor. At `5` it takes `0.1623`, and at `20` it takes `0.4255` and overtakes ligand `1`, whose occupancy has fallen from `0.5760` to `0.3571`: a weak ligand wins by being plentiful.

## Cooperative binding

When the protein has several sites that influence each other, the binding curve becomes sigmoidal. `binding::mwc(L, K_R, c, L0, n)` is the Monod-Wyman-Changeux model for {math}`n` sites, in which the protein switches together between a tense and a relaxed form with the equilibrium ratio {math}`L_0`, and `binding::adair(R_T, L_T, ln_beta)` is the general Adair equation for the mean occupancy per site from the logarithms of the cumulative binding constants. The program computes the MWC curve for a four-site protein with {math}`L_0 = 1000` and {math}`c = 0.01`, the independent-sites curve for comparison (`L0 = 0`), and an Adair curve with strong positive cooperativity.

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

namespace binding = systems::binding;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // cooperative binding to a four-site protein (hemoglobin-like): MWC with n = 4 against four independent sites
  const auto ligand = vec({0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 30.0});
  out.writeln("ligand                       ", flat(ligand));
  out.writeln("MWC, L = 1000, c = 0.01, n=4 ", flat(binding::mwc(ligand, 1.0, 0.01, 1000.0, 4)));
  out.writeln("independent sites (L = 0)    ", flat(binding::mwc(ligand, 1.0, 0.01, 0.0, 4)));
  // the Adair equation with cumulative constants beta_k: ln beta for 4 sites, receptor 1e-3 and the same ligand totals
  const auto ln_beta = vec({std::log(4.0), std::log(6.0 * 4.0), std::log(4.0 * 80.0), std::log(2000.0)});
  out.writeln("Adair mean occupancy         ", flat(binding::adair(vec({1e-3, 1e-3, 1e-3, 1e-3, 1e-3, 1e-3, 1e-3}), ligand, ln_beta.unsqueeze(0).expand({7, 4}).contiguous())));
}
```

Output

```text
ligand                       [0.1000, 0.5000, 1.0000, 2.0000, 5.0000, 10.0000, 30.0000]
MWC, L = 1000, c = 0.01, n=4 [0.0011, 0.0066, 0.0173, 0.0647, 0.4531, 0.8347, 0.9655]
independent sites (L = 0)    [0.0909, 0.3333, 0.5000, 0.6667, 0.8333, 0.9091, 0.9677]
Adair mean occupancy         [0.2997, 0.9101, 0.9590, 0.9799, 0.9920, 0.9960, 0.9987]
```

The MWC curve is sigmoidal: `0.0011` at a ligand of `0.1`, `0.0173` at `1`, `0.4531` at `5` and `0.9655` at `30`, against the hyperbola of independent sites (`0.0909`, `0.5000`, `0.8333`, `0.9677`). The first ligand binds badly, the later ones well, which is cooperativity. The Adair occupancy, with the constants given, rises from `0.2997` to `0.9987`; its constants are for a different, much tighter protein, so only its shape, not its values, can be compared with the MWC curve.

## Decisions

- Use `receptor_ligand` whenever the receptor is not negligible compared with {math}`K_d`; the hyperbola is only right when the ligand is in large excess.
- Use `competitive` for any number of ligands for one receptor; use `mwc` when you believe the protein switches as a whole, and `adair` when you have measured the binding constants site by site.
- Put different totals or constants on the leading axis and solve them together.

## Pitfalls

Every total must be positive, the constants must match the ligands, and the hyperbola is wrong when receptor and constant are comparable. The program tries each.

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
namespace binding = systems::binding;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // every total must be positive
  try { out.writeln("zero second ligand: ", flat(binding::competitive(vec({1.0}), vec({2.0, 0.0}).reshape({1, 2}), vec({1.0, 10.0})).occupancy)); }
  catch (const pluss::Error &e) { out.writeln("zero second ligand: ", e.message()); }
  // the number of dissociation constants must match the number of ligands
  try { out.writeln("two ligands, three Kd: ", flat(binding::competitive(vec({1.0}), vec({2.0, 1.0}).reshape({1, 2}), vec({1.0, 10.0, 5.0})).occupancy)); }
  catch (const pluss::Error &e) { out.writeln("two ligands, three Kd: ", e.message()); }
  // a hyperbola is only right when the receptor is much smaller than Kd: at 100 times Kd the error is large
  const auto exact = binding::receptor_ligand(vec({100.0}), vec({100.0}), 1.0);
  out.writeln("receptor 100 Kd, ligand 100 Kd: exact bound fraction ", flat(exact.div(100.0)), ", hyperbola ", 100.0 / 101.0);
}
```

Output

```text
zero second ligand: natsci: speciation: totals must be strictly positive
zero receptor: [0.0000]
two ligands, three Kd: broadcast: incompatible shapes
receptor 100 Kd, ligand 100 Kd: exact bound fraction [0.9049], hyperbola 0.9901
```

A ligand total of zero is rejected by `competitive` with the message `totals must be strictly positive`; leave an absent ligand out instead. Three constants for two ligands fail with a shape error. With receptor and ligand both a hundred times {math}`K_d` the exact bound fraction is `0.9049` and the hyperbola says `0.9901`, an error of nine percentage points.

## Where next

[Biology 5: Enzymes](../bio_05_enzymes.md) uses the same binding step inside a catalytic cycle, and [Biology 20: Metabolic Control](../bio_20_control.md) asks how a change in a binding constant propagates to a pathway.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
