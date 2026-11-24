# Duplex melting

[Back to Biology 24: Nucleic Acids](../bio_24_nucleic.md)

Two complementary strands of DNA pair into a double helix and heating separates them, at a temperature that depends on the sequence, the length, the salt and the strand concentration. You want that temperature for a primer or a probe before you order it. This page computes the melting temperature from the nearest-neighbour thermodynamics of a sequence, shows its dependence on salt and concentration, and calculates a whole melting curve.

## How it works

The code is in `pluss::ma::natsci::biology::biophysics::nucleic_acids` (`nucleic_acids::` below; the examples shorten it to `na::`). The nearest-neighbour model adds the enthalpy and entropy of every pair of adjacent base pairs, plus terms for the ends. `nucleic_acids::duplex_thermodynamics(sequence, table)` returns the total with the length and GC fraction, and the melting temperature is the point where half the strands are paired:

```{math}
T_m = \frac{\Delta H}{\Delta S + R\ln k} - 273.15\ [^\circ\mathrm{C}], \qquad k = c_1 - c_2/2 .
```

`nucleic_acids::melting_temperature(duplex, c1, c2, salt_method, na, k, tris, mg, dntps)` takes strand concentrations in nM and ion concentrations in mM. The curve of paired strands against temperature comes from the equilibrium constant `nucleic_acids::equilibrium_constant(duplex, T, correction)`, which refers to {math}`1` M sodium unless the entropy correction from `nucleic_acids::salt_correction` is passed, and `nucleic_acids::duplex_fraction(K, cA, cB, self_complementary)`.

## Sequence and length

The program evaluates five sequences, from an all-AT chain to an all-GC chain and a longer one, at {math}`250` nM of each strand and {math}`50` mM sodium.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace na = biology::biophysics::nucleic_acids;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// melting temperature in Celsius of a duplex of the given strand (no other strand in excess), 250 nM of each strand, in 50 mM Na+ (salt correction method 5)
static double melt(const std::string &sequence, double sodium_mm, double strand_nm = 250.0) {
  const auto duplex = na::duplex_thermodynamics(sequence, na::table("DNA_NN4"));
  return flat(na::melting_temperature(duplex, vec({strand_nm}), vec({strand_nm}), 5, vec({sodium_mm}), vec({0.0}), vec({0.0}), vec({0.0}), vec({0.0})))[0];
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // longer strands and strands rich in G and C (three hydrogen bonds each) melt at higher temperature
  for (const std::string seq : {"ATATATATATAT", "ATGCATGCATGC", "GCGCGCGCGCGC", "ATGCGTACGTTAGC", "ATGCGTACGTTAGCATGCGTACGT"}) {
    const auto duplex = na::duplex_thermodynamics(seq, na::table("DNA_NN4"));
    out.writeln(seq, ": length ", duplex.length, ", GC fraction ", duplex.gc_fraction, ", enthalpy ", duplex.dh_kcal, " kcal/mol, melting temperature ", melt(seq, 50.0), " C");
  }
}
```

Output

```text
ATATATATATAT: length 12, GC fraction 0.00, enthalpy -74.60 kcal/mol, melting temperature 8.88 C
ATGCATGCATGC: length 12, GC fraction 0.50, enthalpy -91.10 kcal/mol, melting temperature 40.54 C
GCGCGCGCGCGC: length 12, GC fraction 1.00, enthalpy -111.60 kcal/mol, melting temperature 63.19 C
ATGCGTACGTTAGC: length 14, GC fraction 0.50, enthalpy -109.10 kcal/mol, melting temperature 44.82 C
ATGCGTACGTTAGCATGCGTACGT: length 24, GC fraction 0.50, enthalpy -194.50 kcal/mol, melting temperature 62.00 C
```

The melting temperature rises steeply with the fraction of G and C, from `8.88` {math}`^\circ`C for the all-AT strand to `40.54` for a half-GC strand and `63.19` for the all-GC strand of the same length, and with length, from `44.82` for `14` bases to `62.00` for `24`.

## Salt and concentration

Salt screens the repulsion between the backbones and a higher strand concentration pushes the equilibrium towards the duplex. The program melts one sequence at four sodium concentrations and three strand concentrations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace na = biology::biophysics::nucleic_acids;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// melting temperature in Celsius of a duplex of the given strand (no other strand in excess), 250 nM of each strand, in 50 mM Na+ (salt correction method 5)
static double melt(const std::string &sequence, double sodium_mm, double strand_nm = 250.0) {
  const auto duplex = na::duplex_thermodynamics(sequence, na::table("DNA_NN4"));
  return flat(na::melting_temperature(duplex, vec({strand_nm}), vec({strand_nm}), 5, vec({sodium_mm}), vec({0.0}), vec({0.0}), vec({0.0}), vec({0.0})))[0];
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2).with_sep(""));
  // salt shields the charge of the backbones and stabilises the duplex; strand concentration also matters
  const std::string seq = "ATGCGTACGTTAGC";
  for (double sodium : {10.0, 50.0, 150.0, 1000.0}) out.writeln("sodium ", sodium, " mM: melting temperature ", melt(seq, sodium), " C");
  for (double strand : {10.0, 250.0, 10000.0}) out.writeln("strands at ", strand, " nM each: melting temperature ", melt(seq, 50.0, strand), " C");
}
```

Output

```text
sodium 10.00 mM: melting temperature 37.84 C
sodium 50.00 mM: melting temperature 44.82 C
sodium 150.00 mM: melting temperature 49.77 C
sodium 1000.00 mM: melting temperature 58.68 C
strands at 10.00 nM each: melting temperature 39.00 C
strands at 250.00 nM each: melting temperature 44.82 C
strands at 10000.00 nM each: melting temperature 51.76 C
```

Sodium raises the temperature from `37.84` {math}`^\circ`C at `10` mM to `58.68` at `1000` mM, and a higher strand concentration raises it too, from `39.00` at `10` nM to `51.76` at `10000` nM.

## A melting curve

The program computes the curve of a `14`-base duplex at {math}`1\,\mu`M of each strand in `50` mM sodium, from `30` to `70` {math}`^\circ`C.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace na = biology::biophysics::nucleic_acids;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// melting temperature in Celsius of a duplex of the given strand (no other strand in excess), 250 nM of each strand, in 50 mM Na+ (salt correction method 5)
static double melt(const std::string &sequence, double sodium_mm, double strand_nm = 250.0) {
  const auto duplex = na::duplex_thermodynamics(sequence, na::table("DNA_NN4"));
  return flat(na::melting_temperature(duplex, vec({strand_nm}), vec({strand_nm}), 5, vec({sodium_mm}), vec({0.0}), vec({0.0}), vec({0.0}), vec({0.0})))[0];
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // a melting curve: the fraction of strands in the duplex as the temperature rises, for 1 uM of each strand in 50 mM sodium
  const auto duplex = na::duplex_thermodynamics("ATGCGTACGTTAGC", na::table("DNA_NN4"));
  std::vector<double> t; for (double c = 30.0; c <= 70.0; c += 5.0) t.push_back(c + 273.15);
  // the equilibrium constant refers to 1 M sodium; the salt correction (for 50 mM Na+, method 5) adds an entropy term
  const double correction = flat(na::salt_correction(vec({50.0}), vec({0.0}), vec({0.0}), vec({0.0}), vec({0.0}), 5, duplex))[0];
  const auto k = na::equilibrium_constant(duplex, vec(t), correction);
  const auto duplex_fraction = na::duplex_fraction(k, vec(std::vector<double>(t.size(), 1.0e-6)), vec(std::vector<double>(t.size(), 1.0e-6)), false);
  std::vector<double> celsius; for (double x : t) celsius.push_back(x - 273.15);
  out.writeln("temperature (C)     ", celsius);
  out.writeln("fraction in duplex  ", flat(duplex_fraction));
}
```

Output

```text
temperature (C)     [30.000, 35.000, 40.000, 45.000, 50.000, 55.000, 60.000, 65.000, 70.000]
fraction in duplex  [0.995, 0.978, 0.911, 0.691, 0.269, 0.035, 0.003, 0.000, 0.000]
```

The transition is sharp: `0.995` of the strands are paired at `30` {math}`^\circ`C and `0.911` at `40`; the fraction falls to `0.269` at `50` and has almost entirely gone (`0.003`) by `60`, with the midpoint near `47` {math}`^\circ`C, above the `44.82` {math}`^\circ`C of the lower strand concentration.

## Decisions

- Use `melting_temperature` for one number and `equilibrium_constant` with `duplex_fraction` when you need the shape of the transition or the paired fraction at the working temperature.
- Pick the salt correction method to match your buffer: method `5` adds an entropy term for sodium; the others also account for potassium, Tris, magnesium and dNTPs.
- Give strand concentrations in nM and ions in mM, and pass `true` for self-complementary strands so that the symmetry term is included.

## Pitfalls

Sequences with a letter that is not a base, a single base, and an unknown table name are errors, but a quantity in the wrong unit is not: it is simply a different experiment. The program tries all of them.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace na = biology::biophysics::nucleic_acids;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }
// melting temperature in Celsius of a duplex of the given strand (no other strand in excess), 250 nM of each strand, in 50 mM Na+ (salt correction method 5)
static double melt(const std::string &sequence, double sodium_mm, double strand_nm = 250.0) {
  const auto duplex = na::duplex_thermodynamics(sequence, na::table("DNA_NN4"));
  return flat(na::melting_temperature(duplex, vec({strand_nm}), vec({strand_nm}), 5, vec({sodium_mm}), vec({0.0}), vec({0.0}), vec({0.0}), vec({0.0})))[0];
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  try { (void)na::duplex_thermodynamics("ATGXGT", na::table("DNA_NN4")); } catch (const pluss::Error &e) { out.writeln("a letter that is not a base: ", e.message()); }
  try { (void)na::duplex_thermodynamics("A", na::table("DNA_NN4")); } catch (const pluss::Error &e) { out.writeln("one base: ", e.message()); }
  try { (void)na::table("DNA_NN9"); } catch (const pluss::Error &e) { out.writeln("unknown table: ", e.message()); }
  // concentrations are in nM and salt in mM; a value in the wrong unit is not an error, it is a different experiment
  out.writeln("250 nM of each strand:              ", melt("ATGCGTACGTTAGC", 50.0, 250.0), " C");
  out.writeln("the molar value 2.5e-7 given as nM:  ", melt("ATGCGTACGTTAGC", 50.0, 2.5e-7), " C");
  out.writeln("sodium 50 mM:                       ", melt("ATGCGTACGTTAGC", 50.0), " C");
  out.writeln("sodium 0.05 (molar given as mM):     ", melt("ATGCGTACGTTAGC", 0.05), " C");
}
```

Output

```text
a letter that is not a base: natsci: nucleic_acids: 'X' is not a base (use A, C, G, T or U; spaces and digits are ignored)
one base: natsci: nucleic_acids: a duplex needs at least two bases
unknown table: nucleic_acids: unknown table DNA_NN9
250 nM of each strand:              44.8193 C
the molar value 2.5e-7 given as nM:  10.7487 C
sodium 50 mM:                       44.8193 C
sodium 0.05 (molar given as mM):     16.8850 C
```

The first three are rejected with the offending letter or name. The last four lines are the silent ones: the molar value `2.5e-7` entered as nM melts at `10.7487` {math}`^\circ`C instead of `44.8193`, and `0.05` entered as mM sodium gives `16.8850` instead of `44.8193`.

## Where next

[Biology 15: Binding](../bio_15_binding.md) treats the binding equilibria that the duplex fraction is an example of, and [Biology 17: Protein Folding](../bio_17_folding.md) the same two-state transition for proteins.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
