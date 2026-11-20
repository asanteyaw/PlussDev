<!-- generated from compartment_models.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
