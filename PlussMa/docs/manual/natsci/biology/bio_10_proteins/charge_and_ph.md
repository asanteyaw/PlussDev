# Charge and pH

[Back to Biology 10: Proteins](../bio_10_proteins.md)

A protein's charge depends on the acidity of the solution, because each acidic and basic group gains or loses a proton at its own pH. This page reads the net charge, the pH at which it vanishes and the buffering power of three short peptides from their sequences.

## How it works

`protein_groups(sequences)` reads each sequence and counts the groups that can carry charge (the two ends and the residues K, R, H, D, E, C and Y) with their dissociation constants `pKa` and signs. `charge(counts, pka, sign, ph)` adds up the fractional charge of every group at the given pH, `isoelectric_point` finds the pH at which the net charge is zero, and `buffer_capacity` is the amount of acid or base per unit pH change that the groups can absorb.

## Three peptides

The first peptide is rich in lysine and arginine, which are basic; the second in aspartate and glutamate, which are acidic; the third is mixed. The program prints their isoelectric points, their net charge at three pH values and the buffer capacity at pH `7`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace ionization = biology::biophysics::ionization;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // three short peptides: one rich in lysine and arginine, one rich in aspartate and glutamate, one balanced
  const std::vector<std::string> sequences = {"KKRKRHK", "DDEEDEY", "AGKDEGA"};
  const auto groups = ionization::protein_groups(sequences);
  out.writeln("isoelectric point  ", flat(ionization::isoelectric_point(groups.counts, groups.pka, groups.sign)));
  for (double ph : {2.0, 7.0, 12.0})
    out.writeln("net charge at pH ", ph, "  ", flat(ionization::charge(groups.counts, groups.pka, groups.sign, vec({ph, ph, ph}))));
  out.writeln("buffer capacity at pH 7  ", flat(ionization::buffer_capacity(groups.counts, groups.pka, groups.sign, vec({7.0, 7.0, 7.0}))));
}
```

Output

```text
isoelectric point  [12.032, 3.294, 4.370]
net charge at pH 2.000  [7.972, 0.935, 1.960]
net charge at pH 7.000  [5.843, -6.229, -1.201]
net charge at pH 12.000  [0.040, -7.990, -2.990]
buffer capacity at pH 7  [0.614, 0.451, 0.387]
```

The basic peptide is positive until a very high pH, with an isoelectric point of `12.032`; the acidic peptide is neutral already at `3.294`; the mixed peptide falls in between at `4.370`. At pH `7` the three charges are `5.843`, `-6.229` and `-1.201`, with the sign pattern the sequences suggest, and at pH `2` every peptide is positive (`7.972`, `0.935`, `1.960`) because all the acidic groups have taken up a proton.

## A titration curve

The net charge as a function of pH is the titration curve of the peptide, and its zero crossing is the isoelectric point. The program evaluates the charge of a peptide with acidic, basic and neutral residues at seven pH values and prints its isoelectric point.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace ionization = biology::biophysics::ionization;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // a titration curve: the net charge of one peptide from acidic to basic pH, and its isoelectric point
  const auto groups = ionization::protein_groups({"KDEHCYAGK"});
  std::vector<double> ph, charge;
  for (double p = 1.0; p <= 13.0; p += 2.0) {
    ph.push_back(p);
    charge.push_back(flat(ionization::charge(groups.counts, groups.pka, groups.sign, vec({p})))[0]);
  }
  out.writeln("pH            ", ph);
  out.writeln("net charge    ", charge);
  out.writeln("isoelectric point = ", flat(ionization::isoelectric_point(groups.counts, groups.pka, groups.sign)));
}
```

Output

```text
pH            [1.000, 3.000, 5.000, 7.000, 9.000, 11.000, 13.000]
net charge    [3.996, 3.663, 1.257, -0.162, -1.741, -4.717, -4.997]
isoelectric point = [6.741]
```

The charge falls steadily from `3.996` at pH `1` to `-4.997` at pH `13`, crossing zero between pH `5` and `7`; the isoelectric point is `6.741`. The curve is steepest where the pH passes the pKa of one of its groups, which is where the charge of that group changes.

## Where a buffer works

Buffer capacity, the amount of acid or base absorbed per unit change of pH, is largest near the pKa of a group. The program compares peptides of six histidines, six aspartates and six lysines over seven pH values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace ionization = biology::biophysics::ionization;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // a buffer works best near the pKa of its groups: histidine side chains have pKa about 6
  const auto groups = ionization::protein_groups({"HHHHHH", "DDDDDD", "KKKKKK"});
  for (double p : {3.0, 4.5, 6.0, 7.5, 9.0, 10.5, 12.0})
    out.writeln("pH ", p, ": buffer capacity (His6, Asp6, Lys6) = ", flat(ionization::buffer_capacity(groups.counts, groups.pka, groups.sign, vec({p, p, p}))));
}
```

Output

```text
pH 3.000: buffer capacity (His6, Asp6, Lys6) = [0.409, 1.099, 0.395]
pH 4.500: buffer capacity (His6, Asp6, Lys6) = [0.640, 3.247, 0.211]
pH 6.000: buffer capacity (His6, Asp6, Lys6) = [3.529, 0.296, 0.078]
pH 7.500: buffer capacity (His6, Asp6, Lys6) = [0.969, 0.583, 0.619]
pH 9.000: buffer capacity (His6, Asp6, Lys6) = [0.082, 0.069, 1.210]
pH 10.500: buffer capacity (His6, Asp6, Lys6) = [0.003, 0.002, 2.524]
pH 12.000: buffer capacity (His6, Asp6, Lys6) = [0.000, 0.000, 0.136]
```

Each peptide buffers best near its own side-chain pKa: the histidine peptide at pH `6.0` (`3.529`), the aspartate peptide at `4.5` (`3.247`) and the lysine peptide at `10.5` (`2.524`). Away from these pH values the capacity falls by one or two orders of magnitude, `0.003` for the histidine peptide at pH `10.5`.

## Decisions

- Use `isoelectric_point` when you need the pH at which a protein carries no net charge (for electrophoresis or precipitation), and `charge` when you need the charge at a pH you control.
- Choose a buffer whose group has a pKa within one pH unit of the pH you want: `buffer_capacity` peaks there, as the histidine, aspartate and lysine peptides show.
- Pass all your sequences to one `protein_groups` call and evaluate them together; the lanes are independent and a single pH or one pH per sequence both work.

## Pitfalls

The group table is built from the sequence, so the sequence must be a clean string of the twenty one-letter codes. The program tries an unknown letter, a space and an empty string, shows that lower case is read as upper case, and confirms that the isoelectric point really is where the charge vanishes.

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

namespace ionization = biology::biophysics::ionization;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // the residues are the twenty one-letter codes; lower case is read as upper case, anything else is an error
  const auto upper = ionization::protein_groups({"KKRKRHK"}), lower = ionization::protein_groups({"kkrkrhk"});
  out.writeln("pI of KKRKRHK and kkrkrhk: ", flat(ionization::isoelectric_point(upper.counts, upper.pka, upper.sign)), flat(ionization::isoelectric_point(lower.counts, lower.pka, lower.sign)));
  try { (void)ionization::protein_groups({"KKXRK"}); }
  catch (const pluss::Error &e) { out.writeln("a sequence with X: ", e.message()); }
  try { (void)ionization::protein_groups({"KK RK"}); }
  catch (const pluss::Error &e) { out.writeln("a sequence with a space: ", e.message()); }
  try { (void)ionization::protein_groups({""}); }
  catch (const pluss::Error &e) { out.writeln("an empty sequence: ", e.message()); }
  // the isoelectric point lies where the net charge is zero: check it
  const auto g = ionization::protein_groups({"KKRKRHK", "DDEEDEY"});
  const auto pi = ionization::isoelectric_point(g.counts, g.pka, g.sign);
  out.writeln("net charge at the isoelectric points: ", flat(ionization::charge(g.counts, g.pka, g.sign, pi).abs()));
}
```

Output

```text
pI of KKRKRHK and kkrkrhk: [12.032][12.032]
a sequence with X: natsci: protein_groups: sequences use the twenty one-letter amino-acid codes (A C D E F G H I K L M N P Q R S T V W Y)
a sequence with a space: natsci: protein_groups: sequences use the twenty one-letter amino-acid codes (A C D E F G H I K L M N P Q R S T V W Y)
an empty sequence: natsci: protein_groups: empty sequence
net charge at the isoelectric points: [0.000, 0.000]
```

Lower case gives the same `12.032` as upper case. The `X`, the space and the empty string are all rejected with an error that names the rule, rather than being counted as uncharged residues and quietly moving the isoelectric point. At the isoelectric points of the two peptides the net charge is `0.000`.

## Where next

[Biology 15: Binding](../bio_15_binding.md) treats how a ligand occupies a receptor, which is the next step once you know how a protein is charged, and [Biology 17: Protein Folding](../bio_17_folding.md) asks how stable the folded protein is as the solution changes.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
