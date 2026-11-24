# Ionic strength and activity

[Back to Chemistry 5: Electrochemistry](../chem_05_electrochemistry.md)

In a dilute solution an ion behaves as if its concentration were its activity, but not in a salt solution of ordinary strength: every ion is surrounded by a cloud of the opposite charge, which lowers its effective concentration. This page computes the ionic strength of a solution, the activity coefficient of an ion from the limiting law and from the Davies equation, the mean activity coefficient of a salt, and the effect on a cell potential.

## How it works

The ionic strength of a solution with molalities {math}`m_i` and charges {math}`z_i` is

```{math}
I = \tfrac12\sum_i m_i z_i^2
```

`ionic_strength(molality, charge)` sums along the last axis, so a batch of solutions is one call. The Debye-Hueckel limiting law, `log10_gamma_limiting(I, z)`, gives {math}`\log_{10}\gamma = -A z^2\sqrt I` with {math}`A = 0.5091` for water at {math}`25` C, valid below about {math}`0.01` mol/kg. The Davies equation, `log10_gamma_davies(I, z)`, extends it to a few tenths of a molal:

```{math}
\log_{10}\gamma = -A z^2\left(\frac{\sqrt I}{1 + \sqrt I} - 0.3\,I\right)
```

and `mean_activity_davies(I, z_plus, z_minus)` returns the geometric mean {math}`\gamma_\pm` of the two ions of a salt, the quantity that measurements can reach. The coefficient {math}`A` is an optional last argument for other temperatures or solvents.

## The ionic strength of a solution

The program computes {math}`I` for {math}`0.01` mol/kg NaCl and for {math}`0.01` mol/kg CaCl{math}`_2` (which has {math}`0.01` of {math}`\mathrm{Ca^{2+}}` and {math}`0.02` of {math}`\mathrm{Cl^-}`), then three mixtures of NaCl and Na{math}`_2`SO{math}`_4` with species Na{math}`^+`, Cl{math}`^-` and SO{math}`_4^{2-}` on a leading axis.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ec = pluss::ma::natsci::common::electrochem;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // ionic strength of 0.01 mol/kg NaCl (charges +1, -1) and of 0.01 mol/kg CaCl2 (charges +2, -1, molalities 0.01 and 0.02)
  out.writeln("I(NaCl 0.01 m)  ", flat(ec::ionic_strength(vec({0.01, 0.01}), vec({1.0, -1.0}))));
  out.writeln("I(CaCl2 0.01 m) ", flat(ec::ionic_strength(vec({0.01, 0.02}), vec({2.0, -1.0}))));
  // a mixed solution on a leading axis: three solutions of NaCl + Na2SO4 (species Na+, Cl-, SO4 2-)
  const auto m = pluss::series::make({{0.1, 0.1, 0.0}, {0.1, 0.0, 0.05}, {0.2, 0.1, 0.05}}).to(pluss::ScalarType::Float64);
  out.writeln("I for three mixtures ", flat(ec::ionic_strength(m, vec({1.0, -1.0, -2.0}))));
}
```

Output

```text
I(NaCl 0.01 m)  [0.0100]
I(CaCl2 0.01 m) [0.0300]
I for three mixtures [0.1000, 0.1500, 0.2500]
```

The NaCl solution has `0.0100`, because each ion has charge of magnitude one. The doubly charged calcium makes CaCl{math}`_2` three times larger, `0.0300`, since {math}`\tfrac12(0.01\cdot4 + 0.02\cdot1) = 0.03`. For the mixtures the sulphate counts four times its molality: `0.1000`, `0.1500` and `0.2500`.

## Activity coefficients of single ions

The program evaluates {math}`\gamma` for a monovalent ion by the limiting law and by the Davies equation over four decades of ionic strength, and then for a divalent ion with the Davies equation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ec = pluss::ma::natsci::common::electrochem;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto ionic = vec({0.0001, 0.001, 0.01, 0.1, 0.5});
  out.writeln("ionic strength              ", flat(ionic));
  // gamma for a singly charged ion in the limiting law and in the Davies equation
  out.writeln("gamma+ limiting (z = 1)     ", flat(ec::log10_gamma_limiting(ionic, 1.0).mul(std::log(10.0)).exp()));
  out.writeln("gamma+ Davies   (z = 1)     ", flat(ec::log10_gamma_davies(ionic, 1.0).mul(std::log(10.0)).exp()));
  // a doubly charged ion is pulled in much harder
  out.writeln("gamma Davies    (z = 2)     ", flat(ec::log10_gamma_davies(ionic, 2.0).mul(std::log(10.0)).exp()));
}
```

Output

```text
ionic strength              [0.0001, 0.0010, 0.0100, 0.1000, 0.5000]
gamma+ limiting (z = 1)     [0.9883, 0.9636, 0.8894, 0.6903, 0.4365]
gamma+ Davies   (z = 1)     [0.9885, 0.9650, 0.9021, 0.7816, 0.7336]
gamma Davies    (z = 2)     [0.9548, 0.8673, 0.6622, 0.3731, 0.2897]
```

At {math}`I = 10^{-4}` both laws agree, `0.9883` and `0.9885`. They part company as the solution gets stronger: at {math}`I = 0.1` the limiting law has fallen to `0.6903` and the Davies equation to `0.7816`, which is the value that measurements give for NaCl. A divalent ion is much more affected, `0.3731` at the same strength, because {math}`\log\gamma` scales with {math}`z^2`.

## Mean activity coefficients and the potential of a cell

The mean activity coefficient is evaluated for a 1:1 and a 2:1 salt at four strengths. The program then computes the potential of a one-electron silver couple, {math}`E^\circ = 0.80` V, at {math}`0.001` and {math}`0.1` mol/kg, first with concentrations in the Nernst quotient and then with activities.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ec = pluss::ma::natsci::common::electrochem;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // mean activity coefficient of a 1:1 salt (z+ = 1, z- = -1) and of a 2:1 salt (z+ = 2, z- = -1)
  const auto ionic = vec({0.01, 0.05, 0.1, 0.3});
  out.writeln("ionic strength     ", flat(ionic));
  out.writeln("1:1 salt           ", flat(ec::mean_activity_davies(ionic, 1.0, -1.0)));
  out.writeln("2:1 salt           ", flat(ec::mean_activity_davies(ionic, 2.0, -1.0)));
  // effect on a cell potential: Ag+ at 0.001 and 0.1 mol/kg, activity replaces concentration in the Nernst quotient
  const auto m = vec({0.001, 0.1});
  const auto gam = ec::mean_activity_davies(m, 1.0, -1.0);
  const auto t = vec({298.15, 298.15});
  out.writeln("potential (V), ideal    ", flat(ec::nernst(vec({0.80, 0.80}), t, 1.0, m.reciprocal())));
  out.writeln("potential (V), activity ", flat(ec::nernst(vec({0.80, 0.80}), t, 1.0, m.mul(gam).reciprocal())));
}
```

Output

```text
ionic strength     [0.0100, 0.0500, 0.1000, 0.3000]
1:1 salt           [0.9021, 0.8215, 0.7816, 0.7339]
2:1 salt           [0.6622, 0.4554, 0.3731, 0.2901]
potential (V), ideal    [0.6225, 0.7408]
potential (V), activity [0.6216, 0.7345]
```

The 1:1 salt has {math}`\gamma_\pm` from `0.9021` at {math}`0.01` to `0.7339` at {math}`0.3`, and the 2:1 salt, with {math}`z_+z_- = 2`, falls much faster, to `0.2901`. In the cell the correction is `0.0009` V at {math}`0.001` mol/kg (`0.6225` against `0.6216`) and `0.0063` V at {math}`0.1` mol/kg (`0.7408` against `0.7345`): small, but larger than the precision of a good voltmeter once the solution is no longer dilute.

## Decisions

- Use the limiting law only well below {math}`0.01` mol/kg and the Davies equation up to about {math}`0.5` mol/kg; beyond that neither describes real salts and you need measured activity coefficients.
- Compute {math}`I` from every ion in the solution, including those of a background electrolyte, since the ones you are not interested in often dominate it.
- Use `mean_activity_davies` when the quantity is a salt's behaviour and `log10_gamma_davies` when you need one ion's activity, as in a pH electrode.
- Pass a different {math}`A` for a different temperature or solvent; the default is for water at {math}`25` C.

## Pitfalls

The ionic strength needs one charge per species, and the Davies equation is only a fit for a limited range of strengths. The program shows the error from a missing charge and then runs the activity coefficient into ionic strengths up to {math}`4` mol/kg.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace ec = pluss::ma::natsci::common::electrochem;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the charge vector must have one entry per species
  try {
    ec::ionic_strength(vec({0.1, 0.1, 0.1}), vec({1.0, -1.0}));
  } catch (const pluss::Error &e) {
    out.writeln("wrong charge count: ", e.message());
  }
  // the Davies equation at ionic strength 2 mol/kg: the limiting law keeps falling, Davies turns up
  const auto ionic = vec({0.5, 1.0, 2.0, 4.0});
  out.writeln("ionic strength           ", flat(ionic));
  out.writeln("gamma limiting law       ", flat(ec::log10_gamma_limiting(ionic, 1.0).mul(std::log(10.0)).exp()));
  out.writeln("gamma Davies             ", flat(ec::log10_gamma_davies(ionic, 1.0).mul(std::log(10.0)).exp()));
}
```

Output

```text
wrong charge count: natsci: ionic_strength: one charge per species
ionic strength           [0.5000, 1.0000, 2.0000, 4.0000]
gamma limiting law       [0.4365, 0.3097, 0.1906, 0.0959]
gamma Davies             [0.7336, 0.7910, 1.0168, 1.8686]
```

The wrong number of charges is rejected. Above about {math}`1` mol/kg the Davies equation turns up, `1.0168` at {math}`I = 2` and `1.8686` at {math}`4`, as its {math}`-0.3\,I` term takes over: this is the shape of real activity coefficients of some salts, but the equation has no basis for it, and a result above one should be treated as outside the range of the model. The limiting law keeps falling, `0.0959` at {math}`I = 4`, and is further from reality still.

## Where next

[Cell potentials and the Nernst equation](cell_potentials.md) uses activities in place of concentrations in {math}`Q`, and [Chemistry 6: Aqueous Speciation](../chem_06_speciation.md) finds the concentrations of the ions that make up {math}`I`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="electrode_kinetics.html">
    <span class="label">Previous</span>
    <span class="title">Electrode kinetics and cell polarisation</span>
  </a>
</div>
<!-- apiref:nav:end -->
