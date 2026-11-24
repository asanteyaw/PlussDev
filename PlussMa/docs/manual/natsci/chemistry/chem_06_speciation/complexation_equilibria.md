# Complexation and acid-base equilibria

[Back to Chemistry 6: Aqueous Speciation](../chem_06_speciation.md)

You have a solution with known total amounts of some components, say an acid and a carbonate, or a metal and a ligand, and you want to know how much of each species is actually present and what the pH is. This page describes the equilibria as one stoichiometry matrix and a vector of formation constants, solves for the free concentrations, and sweeps whole titrations and ligand series in a single call.

## How it works

Choose a set of {math}`m` components, the free species out of which everything else is built (for example {math}`\mathrm{H^+}` and {math}`\mathrm{CO_3^{2-}}`). Every other species is a complex {math}`c` formed from them, {math}`\nu_{1c}A_1 + \cdots + \nu_{mc}A_m \rightleftharpoons C_c`, with a formation constant {math}`K_c`. The concentration of each complex follows from the free concentrations {math}`x` by mass action, and the totals {math}`T_i` of the components are conserved:

```{math}
[C_c] = K_c \prod_i x_i^{\nu_{ic}}, \qquad T_i = x_i + \sum_c \nu_{ic}\,[C_c]
```

`speciation::solve(N, ln_K, T)` takes the matrix {math}`N` with one row per component and one column per complex, the natural logarithms of the formation constants and the totals, and solves the {math}`m` balance equations for the logarithms of the free concentrations by Newton's method with a capped step. It returns `free` {math}`[\ldots, m]`, `complexes` {math}`[\ldots, c]`, the largest relative balance error `residual`, the iteration count and `converged`, which is true when every lane met the tolerance; the totals may carry leading batch axes, one solution per lane, with the same {math}`N` and {math}`K`. A stoichiometric coefficient may be negative: the hydroxide {math}`\mathrm{OH^-}` is {math}`-1` of {math}`\mathrm{H^+}`, and the total of {math}`\mathrm{H^+}` is then the proton excess {math}`[\mathrm{H^+}] - [\mathrm{OH^-}] + \ldots`, the acid that has been added to the reference solution. A proton excess may be zero (a pure base) or negative (an excess of base); the total of a component that is a true amount of substance, such as a metal, cannot be.

## A carbonate solution

The components are {math}`\mathrm{H^+}` and {math}`\mathrm{CO_3^{2-}}`, the complexes {math}`\mathrm{OH^-}`, {math}`\mathrm{HCO_3^-}` and {math}`\mathrm{H_2CO_3}` with {math}`\log_{10}K = -14`, {math}`10.33` and {math}`10.33 + 6.35`. A solution of {math}`0.010` mol/L carbonate to which {math}`0.005` mol/L of strong acid has been added is solved, and its pH is printed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace spec = pluss::ma::natsci::common::speciation;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // components: H+ and CO3 2-.  complexes: OH-, HCO3-, H2CO3.  rows = components, columns = complexes
  const auto n = mat({-1, 1, 2,    //  H+  in OH-, HCO3-, H2CO3
                       0, 1, 1}, 2, 3);
  const double ln10 = std::log(10.0);
  const auto ln_k = vec({-14.0 * ln10, 10.33 * ln10, (10.33 + 6.35) * ln10});
  // 0.010 mol/L of carbonate with 0.005 mol/L of strong acid added: totals are [acid, carbonate]
  const auto totals = vec({0.005, 0.010});
  const auto r = spec::solve(n, ln_k, totals);
  const auto h = flat(r.free)[0];
  out.writeln("log10 free H+, CO3 2-  ", flat(r.free.log10()));
  out.writeln("OH-, HCO3-, H2CO3 (mol/L) ", flat(r.complexes));
  out.writeln("pH = ", std::vector<double>{-std::log10(h)});
  out.writeln("largest relative residual ", flat(r.residual), ", iterations ", std::to_string(r.iterations));
}
```

Output

```text
log10 free H+, CO3 2-  [-10.295806, -2.318489]
OH-, HCO3-, H2CO3 (mol/L) [0.000198, 0.005196, 0.000001]
pH = [10.295806]
largest relative residual [0.000000], iterations 37
```

Half of the carbonate has been protonated: `0.005196` mol/L of bicarbonate against `0.004803` of free carbonate, with `0.000198` of hydroxide and only `0.000001` of carbonic acid. The pH is `10.295806`, a little below {math}`\mathrm{p}K_2 = 10.33` as the buffer equation {math}`\mathrm{pH} = \mathrm{p}K_2 + \log([\mathrm{CO_3^{2-}}]/[\mathrm{HCO_3^-}])` says for a ratio slightly under one. The balances close: the carbonate species add to `0.010000` and the residual is `0.000000`. The solver needed `37` iterations, because the starting point is the total and the step is capped.

## A titration in one call

Seven additions of acid to the same carbonate solution are the seven lanes of one `solve`. The totals are a {math}`[7,2]` matrix, and the pH, bicarbonate and carbonic acid of the whole titration come back together.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace spec = pluss::ma::natsci::common::speciation;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto n = mat({-1, 1, 2, 0, 1, 1}, 2, 3);
  const double ln10 = std::log(10.0);
  const auto ln_k = vec({-14.0 * ln10, 10.33 * ln10, (10.33 + 6.35) * ln10});
  // a titration: seven acid additions, each a lane, against 0.010 mol/L of carbonate
  const auto acid = std::vector<double>{0.001, 0.004, 0.0095, 0.010, 0.0105, 0.015, 0.019};
  std::vector<double> t;
  for (double a : acid) { t.push_back(a); t.push_back(0.010); }
  const auto r = spec::solve(n, ln_k, mat(t, 7, 2));
  const auto h = r.free.select(-1, 0);
  out.writeln("acid added (mol/L) ", acid);
  out.writeln("pH                 ", flat(h.log10().neg()));
  out.writeln("HCO3- (mol/L)      ", flat(r.complexes.select(-1, 1)));
  out.writeln("H2CO3 (mol/L)      ", flat(r.complexes.select(-1, 2)));
  out.writeln("largest residual   ", flat(r.residual.max()));
}
```

Output

```text
acid added (mol/L) [0.0010, 0.0040, 0.0095, 0.0100, 0.0105, 0.0150, 0.0190]
pH                 [10.9577, 10.4551, 9.0585, 8.3353, 7.6113, 6.3499, 5.3977]
HCO3- (mol/L)      [0.0019, 0.0043, 0.0095, 0.0098, 0.0095, 0.0050, 0.0010]
H2CO3 (mol/L)      [0.0000, 0.0000, 0.0000, 0.0001, 0.0005, 0.0050, 0.0090]
largest residual   [0.0000]
```

The titration shows the two buffers and the two equivalence regions of a diprotic base. The pH falls from `10.9577` through `10.4551` near the first buffer, to `9.0585` just before the first equivalence at {math}`0.010` mol/L of acid, where it is `8.3353`, the average of the two {math}`\mathrm{p}K` values (`8.34`). It is `6.3499` at {math}`0.015`, the second buffer point, {math}`\mathrm{p}K_1 = 6.35`, and `5.3977` near the end, where carbonic acid, `0.0090` mol/L, has become the main species. All seven residuals are below `0.00005`.

## Metal-ligand complexes

Copper and ammonia form four successive complexes, {math}`\mathrm{Cu(NH_3)}_n^{2+}` with {math}`\log_{10}\beta = 4.04`, {math}`7.47`, {math}`10.27` and {math}`11.75`. The components are {math}`\mathrm{Cu^{2+}}` and {math}`\mathrm{NH_3}`, the stoichiometry is {math}`[1,1,1,1]` for copper and {math}`[1,2,3,4]` for ammonia. Copper is held at {math}`0.001` mol/L, total ammonia runs through six values from {math}`0.002` to {math}`0.1` mol/L, and the fraction of copper in each complex is printed.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace spec = pluss::ma::natsci::common::speciation;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // components: Cu2+ and NH3.  complexes: Cu(NH3)n 2+ for n = 1..4, with log10 beta = 4.04, 7.47, 10.27, 11.75
  const auto n = mat({1, 1, 1, 1,
                      1, 2, 3, 4}, 2, 4);
  const double ln10 = std::log(10.0);
  const auto ln_k = vec({4.04 * ln10, 7.47 * ln10, 10.27 * ln10, 11.75 * ln10});
  // copper fixed at 1e-3 mol/L, total ammonia from 1e-3 to 1e-1 mol/L (free ligand is the unknown)
  const auto ammonia = std::vector<double>{0.002, 0.005, 0.01, 0.02, 0.05, 0.1};
  std::vector<double> t;
  for (double a : ammonia) { t.push_back(0.001); t.push_back(a); }
  const auto r = spec::solve(n, ln_k, mat(t, 6, 2));
  out.writeln("total NH3         ", ammonia);
  out.writeln("log10 free Cu2+  ", flat(r.free.select(-1, 0).log10()));
  out.writeln("free NH3 (mol/L) ", flat(r.free.select(-1, 1)));
  // fraction of copper in each complex
  const auto frac = r.complexes.div(pluss::series::full({1}, 0.001, pluss::ScalarType::Float64));
  out.writeln("fraction as Cu(NH3)  ", flat(frac.select(-1, 0)));
  out.writeln("fraction as Cu(NH3)2 ", flat(frac.select(-1, 1)));
  out.writeln("fraction as Cu(NH3)3 ", flat(frac.select(-1, 2)));
  out.writeln("fraction as Cu(NH3)4 ", flat(frac.select(-1, 3)));
}
```

Output

```text
total NH3         [0.0020, 0.0050, 0.0100, 0.0200, 0.0500, 0.1000]
log10 free Cu2+  [-4.0900, -5.6897, -6.9714, -8.1465, -9.6575, -10.8141]
free NH3 (mol/L) [0.0004, 0.0024, 0.0070, 0.0167, 0.0464, 0.0963]
fraction as Cu(NH3)  [0.3762, 0.0545, 0.0082, 0.0013, 0.0001, 0.0000]
fraction as Cu(NH3)2 [0.4273, 0.3565, 0.1556, 0.0591, 0.0140, 0.0042]
fraction as Cu(NH3)3 [0.1138, 0.5469, 0.6897, 0.6240, 0.4104, 0.2549]
fraction as Cu(NH3)4 [0.0015, 0.0402, 0.1463, 0.3156, 0.5755, 0.7409]
```

Free copper falls by six orders of magnitude, from {math}`10^{-4.09}` to {math}`10^{-10.81}` mol/L, as ammonia is added, because it is almost entirely bound. The complexes appear in order: with only `0.002` mol/L of ammonia the mono and di complexes hold `0.3762` and `0.4273` of the copper, at `0.01` the tri complex leads with `0.6897`, and at `0.1` the tetra complex holds `0.7409`. The free ammonia `0.0963` is close to the total `0.1000` once the copper is saturated, which is the regime in which each fraction depends only on the free ligand.

## Decisions

- Choose components so that every species is a product of components with non-negative totals; the usual choice is the free metal, the free ligand and {math}`\mathrm{H^+}`.
- Put the equilibrium constants in as natural logarithms. To convert a published {math}`\log_{10}K` multiply by {math}`\ln 10`, as the programs do.
- Use leading batch axes of the totals to run a titration or a series of ligand concentrations as one call; the matrix and the constants are shared by all lanes.
- Check `converged` after every solve. Reaching the iteration limit is not an error: the function returns the state it stopped at with `converged = false`, as the other solvers of the library do, and the `residual` says how far off it is.

## Pitfalls

The solver is sound for the cases it can solve and says so when it cannot. The program solves a pure carbonate and an excess of base, which have zero and negative proton totals, then gives a metal a negative total, gives a constant vector that is too short, and limits the iterations to five.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace spec = pluss::ma::natsci::common::speciation;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  const auto n = mat({-1, 1, 2, 0, 1, 1}, 2, 3);
  const double ln10 = std::log(10.0);
  const auto ln_k = vec({-14.0 * ln10, 10.33 * ln10, (10.33 + 6.35) * ln10});
  // a pure carbonate solution has no proton excess: the total of H+ is zero, and the solution is still well defined
  const auto pure = spec::solve(n, ln_k, vec({0.0, 0.010}));
  out.writeln("zero proton excess : converged ", pure.converged ? "yes" : "no", ", pH ", flat(pure.free.select(-1, 0).log10().neg()));
  // an excess of base is a negative proton total
  const auto base = spec::solve(n, ln_k, vec({-0.004, 0.010}));
  out.writeln("excess base        : converged ", base.converged ? "yes" : "no", ", pH ", flat(base.free.select(-1, 0).log10().neg()));
  // a component that is a true amount of substance cannot be negative: no solution exists and converged says so
  const auto copper = spec::solve(mat({1.0, 1.0}, 1, 2), vec({1.0, 2.0}), vec({-1.0}));
  out.writeln("negative metal total: converged ", copper.converged ? "yes" : "no");
  // one formation constant too few
  try {
    spec::solve(n, vec({-14.0 * ln10, 10.33 * ln10}), vec({0.005, 0.010}));
  } catch (const pluss::Error &e) {
    out.writeln("short ln_k         : ", e.message());
  }
  // stopping early: with only 5 iterations the solver returns, flagged as not converged
  spec::Options few;
  few.max_iterations = 5;
  const auto r = spec::solve(n, ln_k, vec({0.005, 0.010}), few);
  out.writeln("5 iterations : converged ", r.converged ? "yes" : "no", ", residual ", flat(r.residual), ", pH ", flat(r.free.select(-1, 0).log10().neg()));
  const auto ok = spec::solve(n, ln_k, vec({0.005, 0.010}));
  out.writeln("default      : converged ", ok.converged ? "yes" : "no", ", residual ", flat(ok.residual), ", pH ", flat(ok.free.select(-1, 0).log10().neg()));
}
```

Output

```text
zero proton excess : converged yes, pH [11.133285]
excess base        : converged yes, pH [11.649108]
negative metal total: converged no
short ln_k         : natsci: speciation: ln_k must have one entry per complex
5 iterations : converged no, residual [87668075136.522964], pH [2.301030]
default      : converged yes, residual [0.000000], pH [10.295806]
```

A pure carbonate has no proton excess, and that is a legitimate state: it converges to a pH of `11.133285`, and an excess of base to `11.649108`. A negative total for a component that is a true amount, here a metal, has no solution, and the answer is `converged no`, not an invented number. A short constant vector is an error. With only five iterations the solver returns `pH 2.301030`, the first capped step from the starting guess, with `converged no` and a residual of about {math}`9\times10^{10}` relative to the totals: not an answer. The converged run gives `10.295806` with a residual `0.000000`. Always test `converged` before reading `free` and `complexes`.

## Where next

[Chemistry 5: Electrochemistry](../chem_05_electrochemistry.md) uses these concentrations as activities in the cell potential, and [Biology 15: Binding](../../biology/bio_15_binding.md) treats the same mass-action balance for a ligand and a protein.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
