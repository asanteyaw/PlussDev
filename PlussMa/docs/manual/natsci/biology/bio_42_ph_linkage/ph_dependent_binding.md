# pH-dependent binding

[Back to Biology 42: Proton Linkage](../bio_42_ph_linkage.md)

Phosphate, amino acids and proteins carry several groups that take up or release a proton, and the number of protons bound changes with pH. You want to know which form dominates at a given pH, how many protons a molecule holds, and how a ligand that binds only some forms, or changes the pKa of a group, binds more or less strongly as the pH moves. This page computes the fractions of a polyprotic acid, the proton count of a protein with independent groups, the pH dependence of an apparent binding constant, and the enthalpy you would measure in a given buffer.

## How it works

The code is in `pluss::ma::natsci::biology::biophysics::linkage` (`linkage::` below). A molecule that can bind up to {math}`n` protons is described by its cumulative protonation constants {math}`\beta_k`, with {math}`\log_{10}\beta_k = \sum_{j\le k}\mathrm{p}K_j'` where {math}`\mathrm{p}K_j'` is the logarithm of the constant for the {math}`j`th proton in the order it binds. The partition function and the fraction of molecules carrying {math}`k` protons are

```{math}
\begin{aligned}
&Z = 1 + \sum_{k=1}^{n}\beta_k\,[\mathrm{H}]^k, \\
&f_k = \frac{\beta_k [\mathrm{H}]^k}{Z}, \\
&\langle k\rangle = \sum_k k f_k = -\frac{\partial \log_{10} Z}{\partial\,\mathrm{pH}}
\end{aligned}
```

`linkage::polyprotic_fractions(log10_beta, ph)` returns the fractions with the unprotonated form first, `linkage::ln_partition_polyprotic` the logarithm of {math}`Z` and `linkage::mean_protons_polyprotic` the mean {math}`\langle k\rangle`. For a protein with independent groups of {math}`N_g` groups of each class {math}`g` and acid constant {math}`\mathrm{p}K_g`, `linkage::ln_partition_independent(counts, pka, ph)` and `linkage::mean_protons_independent(counts, pka, ph)` do the same. The last relation is the Wyman linkage relation: the proton count is the slope of {math}`\log Z` against pH. If a ligand binds and changes the partition function from {math}`Z_{\mathrm{free}}` to {math}`Z_{\mathrm{bound}}`, its apparent association constant at a pH is `linkage::log10_apparent_constant(log10_k_ref, ln_z_bound, ln_z_free)`, which is {math}`\log_{10}K_{\mathrm{ref}} + (\ln Z_{\mathrm{bound}} - \ln Z_{\mathrm{free}})/\ln 10`. `linkage::bound_complex(receptor_total, ligand_total, log10_k_app)` gives the complex concentration at given totals. A buffer takes up the protons that binding releases, so the measured enthalpy is `linkage::observed_enthalpy(dh_intrinsic, protons_taken_up, dh_buffer_ionization)`, the intrinsic enthalpy plus the protons exchanged times the ionisation enthalpy of the buffer.

## The forms of phosphate

Phosphate has three protonation steps. The program lists the fraction of each form at six pH values, with the cumulative constants {math}`\log_{10}\beta = 12.35,\ 19.55,\ 21.70`, and the mean number of bound protons.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace linkage = biology::biophysics::linkage;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // phosphate: cumulative protonation constants log10 beta_k = sum of the first k protonation constants of PO4^3-: 12.35, 19.55, 21.70
  const auto log10_beta = vec({12.35, 19.55, 21.70});
  const auto ph = vec({1.0, 2.15, 4.5, 7.2, 9.0, 12.35});
  const auto f = linkage::polyprotic_fractions(log10_beta, ph);       // [pH, species]: PO4, HPO4, H2PO4, H3PO4
  out.writeln("pH                         ", flat(ph));
  const char *names[] = {"PO4^3-", "HPO4^2-", "H2PO4^-", "H3PO4"};
  for (int s = 0; s < 4; ++s) out.writeln(names[s], std::string(26 - std::string(names[s]).size(), ' '), flat(f.select(-1, s)));
  out.writeln("mean protons bound         ", flat(linkage::mean_protons_polyprotic(log10_beta, ph)));
  out.writeln("sum of fractions           ", flat(f.sum(-1, false)));
}
```

Output

```text
pH                         [1.0000, 2.1500, 4.5000, 7.2000, 9.0000, 12.3500]
PO4^3-                    [0.0000, 0.0000, 0.0000, 0.0000, 0.0004, 0.5000]
HPO4^2-                   [0.0000, 0.0000, 0.0020, 0.5000, 0.9840, 0.5000]
H2PO4^-                   [0.0661, 0.5000, 0.9936, 0.5000, 0.0156, 0.0000]
H3PO4                     [0.9339, 0.5000, 0.0044, 0.0000, 0.0000, 0.0000]
mean protons bound         [2.9339, 2.5000, 2.0025, 1.5000, 1.0152, 0.5000]
sum of fractions           [1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
```

At {math}`\mathrm{pH} = 7.2`, equal to the second {math}`\mathrm{p}K`, the two forms {math}`\mathrm{HPO_4^{2-}}` and {math}`\mathrm{H_2PO_4^-}` are present half and half (`0.5000` each) and the mean number of protons is `1.5000`. At {math}`\mathrm{pH} = 4.5`, between the first and second {math}`\mathrm{p}K`, `0.9936` of the molecules are {math}`\mathrm{H_2PO_4^-}`, and at {math}`\mathrm{pH} = 9` `0.9840` are {math}`\mathrm{HPO_4^{2-}}`. The fractions sum to one at every pH.

## A protein with many groups

A protein has `12` acidic groups (pKa `4.0`), `4` histidines (`6.5`) and `8` lysines (`10.5`). The program computes its mean number of bound protons from pH `3` to `11`, and checks the linkage relation by differentiating the partition function.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace linkage = biology::biophysics::linkage;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a protein with independent titratable groups: counts and pKa of each class (Asp/Glu 4.0, His 6.5, Lys 10.5)
  const auto counts = vec({12.0, 4.0, 8.0});
  const auto pka = vec({4.0, 6.5, 10.5});
  const auto ph = vec({3.0, 5.0, 6.5, 7.4, 9.0, 11.0});
  out.writeln("pH                  ", flat(ph));
  out.writeln("protons bound       ", flat(linkage::mean_protons_independent(counts, pka, ph)));
  out.writeln("ln partition fn.    ", flat(linkage::ln_partition_independent(counts, pka, ph)));
  // the slope of ln Z against pH is minus the number of bound protons (Wyman linkage): check it by a central difference
  const auto up = flat(linkage::ln_partition_independent(counts, pka, ph.add(1e-5))), down = flat(linkage::ln_partition_independent(counts, pka, ph.sub(1e-5)));
  std::vector<double> slope;
  for (std::size_t i = 0; i < up.size(); ++i) slope.push_back(-(up[i] - down[i]) / 2e-5 / std::log(10.0));
  out.writeln("-d log10 Z / d pH   ", slope);
}
```

Output

```text
pH                  [3.0000, 5.0000, 6.5000, 7.4000, 9.0000, 11.0000]
protons bound       [22.9078, 12.9683, 10.0370, 8.4457, 7.7675, 1.9222]
ln partition fn.    [199.1673, 116.3975, 76.4940, 57.5895, 27.8928, 2.1983]
-d log10 Z / d pH   [22.9078, 12.9683, 10.0370, 8.4457, 7.7675, 1.9222]
```

The protein holds `22.9078` protons at pH `3`, `10.0370` at pH `6.5` and only `1.9222` at pH `11`. The last row, minus the slope of {math}`\log_{10}Z` against pH, equals the proton count to four digits at every pH: the same numbers from two directions.

## Binding that takes up a proton

A receptor has one titratable group with pKa `7.0` in the free state and `8.0` when the ligand is bound, so binding favours the protonated form. The program computes the apparent binding constant over a range of pH, the complex that forms at one micromolar of each, and the enthalpy a calorimeter would report in three buffers when `0.6` protons are taken up and the intrinsic enthalpy is `-40` kJ/mol.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace linkage = biology::biophysics::linkage;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a ligand binds a receptor with one titratable group, pKa 7.0 in the free receptor and 8.0 in the complex, so binding takes up a proton at pH between
  const double log10_k_ref = 7.0;                                     // log10 of the association constant of the unprotonated pair
  const auto ph = vec({5.0, 6.0, 7.0, 7.5, 8.0, 9.0, 10.0});
  const auto free_ln_z = linkage::ln_partition_polyprotic(vec({7.0}), ph);          // free receptor: log10 beta_1 = pKa = 7.0
  const auto bound_ln_z = linkage::ln_partition_polyprotic(vec({8.0}), ph);         // complex: pKa = 8.0
  const auto log10_kapp = linkage::log10_apparent_constant(vec({log10_k_ref}), bound_ln_z, free_ln_z);
  out.writeln("pH                          ", flat(ph));
  out.writeln("log10 apparent constant     ", flat(log10_kapp));
  // fraction of the receptor that is complexed with 1 micromolar receptor and 1 micromolar ligand (concentrations in mol/L)
  const auto complex = linkage::bound_complex(vec({1e-6}), vec({1e-6}), log10_kapp);
  out.writeln("complex at 1 uM + 1 uM, uM  ", flat(complex.mul(1e6)));
  // observed enthalpy depends on the buffer, because the proton released or taken up goes to or from the buffer
  const auto dh_int = vec({-40.0});
  for (const auto &[name, dh_buffer] : {std::pair<const char *, double>{"phosphate", 5.1}, {"HEPES", 21.0}, {"Tris", 47.5}})
    out.writeln("observed enthalpy in ", name, ": ", flat(linkage::observed_enthalpy(dh_int, vec({0.6}), vec({dh_buffer})))[0], " kJ/mol (0.6 protons taken up)");
}
```

Output

```text
pH                          [5.0000, 6.0000, 7.0000, 7.5000, 8.0000, 9.0000, 10.0000]
log10 apparent constant     [7.9961, 7.9629, 7.7404, 7.5000, 7.2596, 7.0371, 7.0039]
complex at 1 uM + 1 uM, uM  [0.9045, 0.9009, 0.8739, 0.8373, 0.7914, 0.7394, 0.7309]
observed enthalpy in phosphate: -36.9400 kJ/mol (0.6 protons taken up)
observed enthalpy in HEPES: -27.4000 kJ/mol (0.6 protons taken up)
observed enthalpy in Tris: -11.5000 kJ/mol (0.6 protons taken up)
```

At low pH, where both states are protonated, binding is tight ({math}`\log_{10}K = 7.9961` at pH `5`); at high pH, where neither is, it is the reference `7.0039`. In between the constant falls by one unit as the proton is lost, and the complex falls from `0.9045` to `0.7309` micromolar. The measured enthalpy depends on the buffer: `-36.94` kJ/mol in phosphate, `-27.40` in HEPES and `-11.50` in Tris, because the proton goes into the buffer and the buffer's own ionisation enthalpy is added.

## Decisions

- Use the polyprotic functions for one molecule with a few well-defined steps, and the independent-group functions for a protein with many similar groups, where the counts replace the step constants.
- Build apparent constants from the two partition functions, not by guessing a pKa shift for the ligand: the linkage relation then gives the right number of protons exchanged at every pH.
- When you compare enthalpies measured in different buffers, correct with `observed_enthalpy` before you attribute the difference to the molecule.

## Pitfalls

The constants are cumulative: {math}`\log_{10}\beta_k` for binding {math}`k` protons, not the {math}`\mathrm{p}K` of each step. The program feeds the stepwise values of phosphate in place of the cumulative ones.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace linkage = biology::biophysics::linkage;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // phosphate again, but with the stepwise pKa values 12.35, 7.20 and 2.15 entered where cumulative log10 beta values are expected
  const auto ph = vec({2.15, 7.2, 12.35});
  const auto right = linkage::polyprotic_fractions(vec({12.35, 19.55, 21.70}), ph);
  const auto wrong = linkage::polyprotic_fractions(vec({12.35, 7.20, 2.15}), ph);
  out.writeln("cumulative constants (12.35, 19.55, 21.70): mean protons ", flat(linkage::mean_protons_polyprotic(vec({12.35, 19.55, 21.70}), ph)));
  out.writeln("stepwise values entered by mistake        : mean protons ", flat(linkage::mean_protons_polyprotic(vec({12.35, 7.20, 2.15}), ph)));
  out.writeln("fractions at pH 7.2, right: ", flat(right.select(0, 1)));
  out.writeln("fractions at pH 7.2, wrong: ", flat(wrong.select(0, 1)));
}
```

Output

```text
cumulative constants (12.35, 19.55, 21.70): mean protons [2.5000, 1.5000, 0.5000]
stepwise values entered by mistake        : mean protons [1.0000, 1.0000, 0.5000]
fractions at pH 7.2, right: [0.0000, 0.5000, 0.5000, 0.0000]
fractions at pH 7.2, wrong: [0.0000, 1.0000, 0.0000, 0.0000]
```

With the correct cumulative constants the mean number of protons at pH `2.15`, `7.2` and `12.35` is `2.5000`, `1.5000` and `0.5000`. With the stepwise values `12.35, 7.20, 2.15` entered instead, it is `1.0000`, `1.0000` and `0.5000`, and at pH `7.2` the population is all {math}`\mathrm{HPO_4^{2-}}` (`1.0000`) instead of half and half. The call cannot know which kind of constant you meant, so check one pH where you know the answer.

## Where next

[Biology 10: Proteins](../bio_10_proteins.md) computes the charge and the isoelectric point from the pKa values of the groups, [Biology 15: Binding](../bio_15_binding.md) gives the binding curves that these apparent constants feed, and [Biology 17: Folding](../bio_17_folding.md) treats the stability of a protein.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
