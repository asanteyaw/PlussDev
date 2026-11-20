<!-- generated from charge_and_ph.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

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

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
