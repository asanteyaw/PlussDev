# Compartment models

[Back to Biology 7: Pharmacokinetics](../bio_07_pharmacokinetics.md)

After a dose, a drug moves between the blood and the tissues and is cleared from the body. A compartment model treats each region as one well-mixed pool and describes the flow between pools with rates. This page follows a dose through blood and tissue, reads the total exposure and the half-life, finds the level reached by regular dosing, and compares with oral absorption.

## How it works

`rate_matrix(transfer, elimination)` builds the linear system `dx/dt = K x` from the transfer rates between compartments (entry `(i, j)` is the rate of moving from `i` to `j`) and the elimination rate out of each compartment. Because the system is linear, `response(k, dose, times)` returns the amounts at every requested time exactly, through the matrix exponential, for any number of times and doses at once. `auc_infinity` is the total exposure, the area under each amount curve to infinity. `two_compartment_macro` gives the fast and slow decay rates `alpha` and `beta` of a blood-and-tissue model, `half_life` turns a rate into a half-life, `steady_state_trough` is the lowest amount just before each dose when the same dose is repeated every `tau` hours, and `bateman` is the amount in the central compartment after an oral dose absorbed at rate `ka`.

## A dose through blood and tissue

Drug leaves the blood for the tissue at `0.4` per hour, returns at `0.2` per hour, and is eliminated from the blood at `0.3` per hour. The program doses `100` into the blood and prints the amounts at six times and the total exposure, then the macro rates, the half-life of the slow phase, the trough of a twice-daily regimen, and the central amount after an oral dose.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace pk = biology::pharmacokinetics;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // two compartments: blood (0) and tissue (1). Drug moves blood -> tissue at 0.4/h, tissue -> blood at 0.2/h,
  // and is eliminated from blood at 0.3/h.
  const auto transfer = mat({{0.0, 0.4}, {0.2, 0.0}});
  const auto k = pk::rate_matrix(transfer, vec({0.3, 0.0}));
  const auto dose = vec({100.0, 0.0});
  const auto times = vec({0.0, 1.0, 2.0, 4.0, 8.0, 24.0});
  const auto amounts = pk::response(k, dose, times);   // [times, compartments]
  out.writeln("hours   ", flat(times));
  out.writeln("blood   ", flat(amounts.select(-1, 0)));
  out.writeln("tissue  ", flat(amounts.select(-1, 1)));
  out.writeln("total exposure (amount x hours), blood and tissue = ", flat(pk::auc_infinity(k, dose)));

  // the two disposition rates and the terminal half-life
  const auto macro = pk::two_compartment_macro(vec({0.3}), vec({0.4}), vec({0.2}));
  out.writeln("fast rate alpha = ", flat(macro.alpha), ", slow rate beta = ", flat(macro.beta));
  out.writeln("terminal half-life (h) = ", flat(pk::half_life(macro.beta)));

  // repeated doses: the lowest amount just before each dose once the pattern has settled
  out.writeln("trough with a 100 dose every 12 h = ", flat(pk::steady_state_trough(k, dose, 12.0)));
  // a single oral dose absorbed at 1.5/h and eliminated at 0.3/h
  out.writeln("oral dose, central amount at 1, 2, 4, 8 h = ", flat(pk::bateman(vec({1.0, 2.0, 4.0, 8.0}), 100.0, 1.5, 0.3)));
}
```

Output

```text
hours   [0.0000, 1.0000, 2.0000, 4.0000, 8.0000, 24.0000]
blood   [100.0000, 52.0381, 30.4897, 15.6704, 9.5650, 2.9634]
tissue  [0.0000, 26.1152, 35.7048, 37.7079, 29.5915, 9.2975]
total exposure (amount x hours), blood and tissue = [333.3333, 666.6667]
fast rate alpha = [0.8275], slow rate beta = [0.0725]
terminal half-life (h) = [9.5596]
trough with a 100 dose every 12 h = [12.1777, 38.1918]
oral dose, central amount at 1, 2, 4, 8 h = [64.7110, 62.3781, 37.3394, 11.3390]
```

The blood amount falls steeply at first, `100` to `52.04` in an hour, while the tissue fills to a maximum near `4` hours (`37.71`) and then drains slowly: that is the two-phase decay of the fast rate `0.8275` and the slow rate `0.0725`, which gives a terminal half-life of `9.56` hours. The total exposure is `333.33` in the blood and `666.67` in the tissue. A dose of `100` every `12` hours settles to troughs of `12.18` in the blood and `38.19` in the tissue. For an oral dose the central amount rises from nothing, is `64.71` at one hour and `62.38` at two, and decays afterwards: absorption and elimination compete.

<!-- addex:start -->
## A constant infusion

A drip adds drug at a constant rate. `pharmacokinetics::infusion_response(k, x0, u, times)` returns the amounts at each time under a constant input `u`, again exactly. The program delivers `10` units per hour into the blood of the two-compartment model, starting empty, and compares the long-run amounts with the plateau that solves {math}`K x + u = 0`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace pk = biology::pharmacokinetics;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // blood (0) and tissue (1) as before; a drip delivers 10 units per hour into the blood
  const auto k = pk::rate_matrix(mat({{0.0, 0.4}, {0.2, 0.0}}), vec({0.3, 0.0}));
  const auto times = vec({0.0, 2.0, 6.0, 12.0, 24.0, 72.0});
  const auto drip = pk::infusion_response(k, vec({0.0, 0.0}), vec({10.0, 0.0}), times);
  out.writeln("hours   ", flat(times));
  out.writeln("blood   ", flat(drip.select(-1, 0)));
  out.writeln("tissue  ", flat(drip.select(-1, 1)));
  // the plateau of a constant infusion solves K x + u = 0
  out.writeln("long-run amounts (blood, tissue) = ", flat(dynamics::solve(k.neg(), vec({10.0, 0.0}))));
}
```

Output

```text
hours   [0.000, 2.000, 6.000, 12.000, 24.000, 72.000]
blood   [0.000, 11.269, 18.190, 23.577, 29.246, 33.207]
tissue  [0.000, 4.685, 19.419, 36.058, 53.844, 66.272]
long-run amounts (blood, tissue) = [33.333, 66.667]
```

The blood amount rises from `0` to `11.269` after two hours and `29.246` after a day, and approaches `33.207` after three days; the tissue, which fills more slowly, passes `53.844` after a day and `66.272` after three days. They are heading to the plateau `33.333` and `66.667`, found directly from the rate matrix.

## Oral absorption

For a dose taken by mouth the drug first has to be absorbed. `bateman(t, dose, ka, ke)` is the amount in the blood for first-order absorption at rate `ka` and elimination at rate `ke`, and `half_life(rate)` converts a rate to a half-life. The program compares three absorption rates, plus the special case `ka = ke`, which has a formula of its own.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace pk = biology::pharmacokinetics;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // oral doses of 100 with three absorption rates ka, all eliminated at ke = 0.3 per hour: the concentration curve for each, at once
  const auto t = vec({0.5, 1.0, 2.0, 3.0, 4.0, 6.0, 10.0, 16.0});
  out.writeln("hours       ", flat(t));
  for (double ka : {0.5, 1.5, 6.0}) out.writeln("ka = ", ka, "   ", flat(pk::bateman(t, 100.0, ka, 0.3)));
  // the special case ka = ke has its own formula, dose ka t exp(-ka t)
  out.writeln("ka = ke = 0.3 ", flat(pk::bateman(t, 100.0, 0.3, 0.3)));
  out.writeln("elimination half-life for ke = 0.3 (hours) = ", flat(pk::half_life(vec({0.3}))));
}
```

Output

```text
hours       [0.500, 1.000, 2.000, 3.000, 4.000, 6.000, 10.000, 16.000]
ka = 0.500   [20.477, 33.572, 45.233, 45.860, 41.465, 28.878, 10.762, 1.974]
ka = 1.500   [48.543, 64.711, 62.378, 49.433, 37.339, 20.647, 6.223, 1.029]
ka = 6.000   [85.360, 77.720, 57.769, 42.797, 31.705, 17.400, 5.241, 0.866]
ka = ke = 0.3 [12.911, 22.225, 32.929, 36.591, 36.143, 29.754, 14.936, 3.950]
elimination half-life for ke = 0.3 (hours) = [2.310]
```

A slow absorption (`ka = 0.5`) gives a low, late peak, `45.860` at three hours; `ka = 1.5` peaks near one hour at about `64.7`; a fast one (`ka = 6`) is already at `85.360` after half an hour and then decays. The case `ka = ke` is handled without a division by zero and peaks at `36.591` after three hours. The elimination half-life for `ke = 0.3` is `2.310` hours.

## Choosing a dosing interval

`steady_state_trough(k, dose, tau)` gives the lowest amount just before a dose once the pattern has settled, for any interval {math}`\tau`. The program tries doses of `100` every `6`, `12` and `24` hours, and also prints the total exposure of one dose.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace pk = biology::pharmacokinetics;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const auto k = pk::rate_matrix(mat({{0.0, 0.4}, {0.2, 0.0}}), vec({0.3, 0.0}));
  const auto dose = vec({100.0, 0.0});
  // the lowest amount reached under a repeated dose depends on the dosing interval
  for (double tau : {6.0, 12.0, 24.0}) out.writeln("dose every ", tau, " h: trough (blood, tissue) = ", flat(pk::steady_state_trough(k, dose, tau)));
  // one dose: the total exposure and where it ends up
  out.writeln("total exposure of one dose (blood, tissue) = ", flat(pk::auc_infinity(k, dose)));
}
```

Output

```text
dose every 6.000 h: trough (blood, tissue) = [31.567, 96.834]
dose every 12.000 h: trough (blood, tissue) = [12.178, 38.192]
dose every 24.000 h: trough (blood, tissue) = [3.594, 11.276]
total exposure of one dose (blood, tissue) = [333.333, 666.667]
```

Shortening the interval raises the trough sharply: in the blood it is `3.594` for a daily dose, `12.178` for a twelve-hourly dose and `31.567` for one every six hours, and the tissue is higher in each case (`11.276`, `38.192`, `96.834`). The total exposure of a single dose, `333.333` in the blood and `666.667` in the tissue, does not depend on the interval.

<!-- addex:end -->

## Decisions

- Use `response` for any number of times and doses at once; it is exact because the system is linear.
- Use `infusion_response` for a drip, `steady_state_trough` for repeated doses and `bateman` for a single oral dose.
- Check the sign of every elimination before looking at curves: rates are positive numbers.

## Pitfalls

In the transfer matrix entry `(i, j)` is the rate of moving from compartment `i` to compartment `j`. Transposing it sends the drug the other way. The program doses `100` into the blood and prints the amounts for the page's matrix and its transpose.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace pk = biology::pharmacokinetics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // entry (i, j) of the transfer matrix is the rate of moving from i to j: transposing it sends the drug the wrong way
  const auto times = vec({1.0, 4.0, 12.0});
  for (const auto &transfer : {mat({{0.0, 0.4}, {0.2, 0.0}}), mat({{0.0, 0.2}, {0.4, 0.0}})}) {
    const auto k = pk::rate_matrix(transfer, vec({0.3, 0.0}));
    const auto amounts = pk::response(k, vec({100.0, 0.0}), times);
    out.writeln("blood ", flat(amounts.select(-1, 0)), ", tissue ", flat(amounts.select(-1, 1)), ", exposure ", flat(pk::auc_infinity(k, vec({100.0, 0.0}))));
  }
}
```

Output

```text
blood [52.0381, 15.6704, 7.0780], tissue [26.1152, 37.7079, 22.1918], exposure [333.3333, 666.6667]
blood [63.1790, 24.6109, 5.8647], tissue [12.9286, 16.3313, 4.9322], exposure [333.3333, 166.6667]
```

With the page's matrix the tissue fills to `37.7079` by hour `4` and the exposure in the tissue is `666.6667`. Transposed, the tissue reaches only `16.3313` at hour `4` and its exposure is `166.6667`, while the blood exposure is the same `333.3333` in both. A tissue that fills too slowly or too little is the symptom of a transposed matrix.

## Where next

[The SIR model](../bio_06_epidemics/sir_model.md) is a nonlinear flow between compartments, and [Biology 16: Membranes](../bio_16_membranes.md) models the transfer across a membrane.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
