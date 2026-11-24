# Pathway feasibility

[Back to Biology 35: Biochemical Thermodynamics](../bio_35_thermodynamics.md)

A pathway can run only if every reaction has a driving force in the direction of the flux, and the driving force depends on the concentrations the cell can reach. You want to know whether a pathway is thermodynamically feasible, which step is the bottleneck, how a fixed ratio such as ATP to ADP changes things, and what flux a network can carry once the second law is imposed. This page computes the max-min driving force of a pathway and its shadow prices, sweeps a bottleneck, and solves a flux balance with thermodynamic constraints.

## How it works

For a pathway with stoichiometric matrix $S$ (compounds by reactions, oriented along the flux), transformed standard energies $\Delta_r G'^{\circ}$ and log concentrations $x = \ln c$, the driving force of reaction $j$ is $-\Delta_r G'_j = -(\Delta_r G'^{\circ}_j + RT\,(S^{\top}x)_j)$. The *max-min driving force* (MDF) maximises the smallest driving force:

```{math}
\max_{x,\,B}\ B \quad\text{subject to}\quad -\bigl(\Delta_r G'^{\circ} + RT\,S^{\top}x\bigr) \ge B,\quad \ln c_{\min}\le x\le \ln c_{\max}
```

The pathway is feasible exactly when the optimum $B$ is positive. `pathway_thermodynamics::max_min_driving_force(pathway)` solves this linear program for a `Pathway` (fields `stoichiometry`, `dg0` in kJ/mol, `c_min`, `c_max` in M, optional `ratios` that fix the ratio of two compounds, and `temperature`) and returns `mdf`, the log concentrations, the `driving_force` of each step and the `shadow_price` of each step, which is positive for the steps that limit the MDF. `pathway_thermodynamics::thermodynamic_flux_balance(net)` combines flux balance with the second law: reactions marked `constrained` may carry a forward flux only when $\Delta G < 0$ and a reverse flux only when $\Delta G > 0$, with the sign chosen by a binary variable, and the whole is solved as a mixed-integer program over fluxes and concentrations. Energies are in kJ/mol here, concentrations in M with a 1 M standard state.

## The max-min driving force

The program takes a three-step pathway $A\to B\to C\to D$ with standard energies `-5`, `+10` and `-20` kJ/mol, concentrations confined to the range from $10^{-6}$ to $10^{-2}$ M, and computes the MDF, the driving forces, the concentrations that achieve it and the shadow prices.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace thermo = biology::metabolism::thermodynamics;
namespace pathway = biology::metabolism::pathway_thermodynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a three-step pathway A -> B -> C -> D with standard reaction energies -5, +10 and -20 kJ/mol, every concentration between 1 microM and 10 mM
  pathway::Pathway p;
  p.stoichiometry = mat({{-1.0, 0.0, 0.0}, {1.0, -1.0, 0.0}, {0.0, 1.0, -1.0}, {0.0, 0.0, 1.0}});
  p.dg0 = vec({-5.0, 10.0, -20.0});
  p.c_min = {1e-6, 1e-6, 1e-6, 1e-6};
  p.c_max = {1e-2, 1e-2, 1e-2, 1e-2};
  const auto r = pathway::max_min_driving_force(p);
  out.writeln("feasible (MDF > 0)          ", std::vector<double>{r.mdf > 0.0 ? 1.0 : 0.0});
  out.writeln("max-min driving force, kJ/mol ", std::vector<double>{r.mdf});
  out.writeln("driving force of each step    ", r.driving_force);
  out.writeln("log10 of the concentrations (M) at the optimum ", std::vector<double>{r.ln_concentration[0] / std::log(10.0), r.ln_concentration[1] / std::log(10.0), r.ln_concentration[2] / std::log(10.0), r.ln_concentration[3] / std::log(10.0)});
  out.writeln("shadow prices (bottleneck weights) ", r.shadow_price);
}
```

Output

```text
feasible (MDF > 0)          [1.0000]
max-min driving force, kJ/mol [8.9160]
driving force of each step    [8.9160, 8.9160, 20.0000]
log10 of the concentrations (M) at the optimum [-2.0000, -2.6861, -6.0000, -6.0000]
shadow prices (bottleneck weights) [0.5000, 0.5000, 0.0000]
```

The pathway is feasible, with an MDF of `8.916` kJ/mol, although its middle step is uphill by `10` kJ/mol at standard conditions: concentrations make up for it. The first two steps both sit at the minimum `8.916` and the last has `20.000`, more than it needs. The optimal concentrations put A at $10^{-2}$ M (the maximum) and D at $10^{-6}$ M (the minimum), with B at $10^{-2.69}$ M. The shadow prices `0.5, 0.5, 0` name the first two steps as the bottleneck, and the third step plays no role.

## Making one step harder

The program raises the standard energy of the middle step from `0` to `40` kJ/mol and prints the MDF and the shadow prices at each value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace thermo = biology::metabolism::thermodynamics;
namespace pathway = biology::metabolism::pathway_thermodynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  pathway::Pathway p;
  p.stoichiometry = mat({{-1.0, 0.0, 0.0}, {1.0, -1.0, 0.0}, {0.0, 1.0, -1.0}, {0.0, 0.0, 1.0}});
  p.c_min = {1e-6, 1e-6, 1e-6, 1e-6};
  p.c_max = {1e-2, 1e-2, 1e-2, 1e-2};
  // make the middle step harder and harder: its standard energy rises from 0 to 40 kJ/mol
  for (double middle : {0.0, 10.0, 20.0, 30.0, 40.0}) {
    p.dg0 = vec({-5.0, middle, -20.0});
    const auto r = pathway::max_min_driving_force(p);
    out.writeln("middle step ", std::vector<double>{middle}, " kJ/mol   MDF ", std::vector<double>{r.mdf}, "   shadow prices ", r.shadow_price);
  }
}
```

Output

```text
middle step [0.0000] kJ/mol   MDF [13.9160]   shadow prices [0.5000, 0.5000, 0.0000]
middle step [10.0000] kJ/mol   MDF [8.9160]   shadow prices [0.5000, 0.5000, 0.0000]
middle step [20.0000] kJ/mol   MDF [2.8320]   shadow prices [0.0000, 1.0000, 0.0000]
middle step [30.0000] kJ/mol   MDF [-7.1680]   shadow prices [0.0000, 1.0000, 0.0000]
middle step [40.0000] kJ/mol   MDF [-17.1680]   shadow prices [0.0000, 1.0000, 0.0000]
```

The MDF falls from `13.916` to `-17.168` kJ/mol as the middle step becomes harder, linearly in the end: `2.832` at `20`, then `-7.168` at `30`. It changes sign between `20` and `30`, which is where the pathway becomes infeasible. The shadow prices show who limits: when the middle step is at `0` or `10`, the first two steps share the bottleneck (`0.5, 0.5, 0`), and from `20` upward the middle step alone (`0, 1, 0`).

## Fixing a concentration ratio

The program fixes $[A]/[D]$ at `1000`, `1` and `0.001`, the way a cell holds a ratio such as ATP/ADP, and prints the MDF and the driving forces.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace thermo = biology::metabolism::thermodynamics;
namespace pathway = biology::metabolism::pathway_thermodynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  pathway::Pathway p;
  p.stoichiometry = mat({{-1.0, 0.0, 0.0}, {1.0, -1.0, 0.0}, {0.0, 1.0, -1.0}, {0.0, 0.0, 1.0}});
  p.dg0 = vec({-5.0, 10.0, -20.0});
  p.c_min = {1e-6, 1e-6, 1e-6, 1e-6};
  p.c_max = {1e-2, 1e-2, 1e-2, 1e-2};
  const auto free = pathway::max_min_driving_force(p);
  // fix the ratio of the first to the last compound, like an ATP/ADP ratio that the cell holds constant: [A]/[D] = 0.001, then 1000
  for (double ratio : {1000.0, 1.0, 0.001}) {
    pathway::Pathway q = p;
    q.ratios.push_back({0, 3, ratio});
    const auto r = pathway::max_min_driving_force(q);
    out.writeln("[A]/[D] fixed at ", std::vector<double>{ratio}, "   MDF ", std::vector<double>{r.mdf}, "  driving forces ", r.driving_force);
  }
  out.writeln("with no ratio the MDF is ", std::vector<double>{free.mdf});
}
```

Output

```text
[A]/[D] fixed at [1000.0000]   MDF [8.9160]  driving forces [8.9160, 8.9160, 14.2920]
[A]/[D] fixed at [1.0000]   MDF [5.0000]  driving forces [5.0000, 5.0000, 5.0000]
[A]/[D] fixed at [0.0010]   MDF [-0.7080]  driving forces [-0.7080, -0.7080, -0.7080]
with no ratio the MDF is [8.9160]
```

With no constraint the MDF is `8.916`. Fixing $[A]/[D] = 1000$, which the optimum nearly has anyway, costs nothing: `8.916`. A ratio of `1` leaves `5.000` kJ/mol, and every step then has exactly `5.000`: the total driving force is fixed by the ratio, $15$ kJ/mol here (the standard energies add to $-15$ and $RT\ln 1 = 0$), and the optimum shares it equally among the three steps. A ratio of `0.001` gives `-0.708`: the total becomes $-15 + RT\ln 1000 = +2.1$ kJ/mol, uphill, so the pathway cannot run when the substrate is held a thousand times lower than the product.

## Decisions

- Use the MDF to screen pathways before you build flux models: a negative MDF says the pathway cannot run whatever the enzymes do.
- Read the shadow prices to find which steps to look at, in the kinetic or in the regulatory analysis: they are non-zero only for the steps that are at the minimum.
- Add `ratios` for the couples whose ratio the cell holds fixed, such as ATP/ADP or NADH/NAD, and set the concentration bounds to the physiological range you trust.
- Use `thermodynamic_flux_balance` when flux and thermodynamics interact, and mark exchange and biomass reactions as unconstrained.

## Pitfalls

An infeasible pathway is not an error: the MDF is negative. A mismatch between the number of concentration bounds and the number of compounds is refused. The program tries both.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace thermo = biology::metabolism::thermodynamics;
namespace pathway = biology::metabolism::pathway_thermodynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // an impossible pathway: every step is uphill by 60 kJ/mol and the concentrations are confined to a narrow window
  pathway::Pathway p;
  p.stoichiometry = mat({{-1.0, 0.0}, {1.0, -1.0}, {0.0, 1.0}});
  p.dg0 = vec({60.0, 60.0});
  p.c_min = {1e-3, 1e-3, 1e-3};
  p.c_max = {1e-2, 1e-2, 1e-2};
  const auto r = pathway::max_min_driving_force(p);
  out.writeln("MDF ", std::vector<double>{r.mdf}, "  feasible ", r.mdf > 0.0 ? "yes" : "no");
  out.writeln("driving forces ", r.driving_force);
  // wrong number of concentration bounds
  pathway::Pathway bad = p;
  bad.c_min = {1e-3, 1e-3};
  try {
    pathway::max_min_driving_force(bad);
  } catch (const pluss::Error &e) {
    out.writeln("two bounds for three compounds: ", e.message());
  }
}
```

Output

```text
MDF [-57.1460]  feasible no
driving forces [-57.1460, -57.1460]
two bounds for three compounds: natsci: mdf: one concentration bound per compound
```

A pathway of two steps, each uphill by `60` kJ/mol, with concentrations confined to a factor of ten, has an MDF of `-57.146`: infeasible, and the driving forces are both `-57.146`. Only a bound list of the wrong length is refused. Test the sign of `mdf` before you use the concentrations. 

A network whose conversion is thermodynamically forced backwards cannot carry the flux. The program solves the thermodynamic flux balance of an uptake, a conversion and a biomass step for four standard energies of the conversion.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace thermo = biology::metabolism::thermodynamics;
namespace pathway = biology::metabolism::pathway_thermodynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the same network with an uphill conversion: Delta G0 = +30 kJ/mol. Concentrations can change the sign of Delta G, but only within their window
  for (double dg0 : {-10.0, 10.0, 30.0, 50.0}) {
    pathway::TfaNetwork net;
    net.stoichiometry = mat({{1.0, -1.0, 0.0}, {0.0, 1.0, -1.0}});
    net.dg0 = vec({0.0, dg0, 0.0});
    net.lower = {0.0, -100.0, 0.0};
    net.upper = {10.0, 100.0, 100.0};
    net.constrained = {0, 1, 0};
    net.c_min = {1e-6, 1e-6};
    net.c_max = {1e-2, 1e-2};
    net.objective = {0.0, 0.0, 1.0};
    const auto r = pathway::thermodynamic_flux_balance(net);
    out.writeln("Delta G0 = ", std::vector<double>{dg0}, " kJ/mol  biomass ", std::vector<double>{r.objective}, "  Delta G of the conversion ", std::vector<double>{r.delta_g[1]}, "  direction ", std::vector<double>{static_cast<double>(r.forward[1])});
  }
}
```

Output

```text
Delta G0 = [-10.0000] kJ/mol  biomass [10.0000]  Delta G of the conversion [-32.8320]  direction [1.0000]
Delta G0 = [10.0000] kJ/mol  biomass [10.0000]  Delta G of the conversion [-0.0000]  direction [1.0000]
Delta G0 = [30.0000] kJ/mol  biomass [0.0000]  Delta G of the conversion [7.1680]  direction [0.0000]
Delta G0 = [50.0000] kJ/mol  biomass [0.0000]  Delta G of the conversion [27.1680]  direction [0.0000]
```

With $\Delta G^{\circ} = -10$ and $+10$ kJ/mol the network makes `10` units of biomass, and the conversion runs forward; at $+10$ the optimiser needs only the sign of $\Delta G$ and reports a driving force just below zero, `-0.0000`, within the margin of $10^{-6}$ kJ/mol. At $+30$ and $+50$ kJ/mol, $\Delta G$ would be `+7.17` or `+27.17` kJ/mol in the best case, the conversion cannot run forward, and the biomass is `0.0000`: the second law removes a flux that ordinary flux balance would allow.

## Where next

Go back to [Transformed energies and equilibria](transformed_energies.md) for the energies, and to [Biology 4: Metabolism](../bio_04_metabolism.md) and [Biology 33: Loopless Flux Analysis](../bio_33_loopless.md) for the flux side.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transformed_energies.html">
    <span class="label">Previous</span>
    <span class="title">Transformed energies and equilibria</span>
  </a>
</div>
<!-- apiref:nav:end -->
