# Absorption and the Beer-Lambert law

[Back to Chemistry 7: Spectroscopy](../chem_07_spectroscopy.md)

You have measured how much light a sample absorbs and you want the concentrations behind it, or you have the concentrations and want to predict the spectrum. This page computes the absorbance of one absorber or a mixture, the transmittance that goes with it, and recovers the concentrations of a mixture from absorbances at several wavelengths.

## How it works

Light of intensity {math}`I_0` that crosses a path {math}`\ell` of a solution with absorber concentrations {math}`c_j` and molar absorptivities {math}`\varepsilon_j` comes out with intensity {math}`I`, where

```{math}
\begin{aligned}
&A = \log_{10}\frac{I_0}{I} = \ell\sum_j \varepsilon_j c_j, \\
&T = \frac{I}{I_0} = 10^{-A}
\end{aligned}
```

`spectroscopy::absorbance(eps, c, path)` sums over the last axis, which is the species axis, so absorptivities with shape {math}`[\ldots, \text{species}]` and concentrations of the same trailing size give one absorbance for each lane. `transmittance(A)` converts it to {math}`T`. The path length is a number or a Series, so cells of different lengths are a batch like any other. Because the absorbance is linear in the concentrations, an unknown mixture is a linear system: absorbances at {math}`n` wavelengths of {math}`n` absorbers give the concentrations as the solution of {math}`A = \ell\,E\,c`, with {math}`E` the matrix of absorptivities.

## One absorber over a range of concentrations

The absorptivity is {math}`1.2\times10^{4}` L mol{math}`^{-1}` cm{math}`^{-1}` and the cell is {math}`1` cm. Five concentrations from {math}`2\ \mu`M to {math}`50\ \mu`M are five lanes with one species each. The program prints the absorbance and transmittance, then the absorbance in a {math}`2` cm cell.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace sp = pluss::ma::natsci::chemistry::spectroscopy;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // one absorber, molar absorptivity 1.2e4 L/(mol cm), 1 cm cell: five concentrations, one per lane
  const auto conc = mat({2e-6, 5e-6, 1e-5, 2e-5, 5e-5}, 5, 1);
  const auto a = sp::absorbance(vec({1.2e4}), conc, 1.0);
  out.writeln("concentration (mol/L)  ", flat(conc));
  out.writeln("absorbance             ", flat(a));
  out.writeln("transmittance          ", flat(sp::transmittance(a)));
  // doubling the path length doubles the absorbance
  out.writeln("absorbance, 2 cm cell  ", flat(sp::absorbance(vec({1.2e4}), conc, 2.0)));
}
```

Output

```text
concentration (mol/L)  [0.000002, 0.000005, 0.000010, 0.000020, 0.000050]
absorbance             [0.024000, 0.060000, 0.120000, 0.240000, 0.600000]
transmittance          [0.946237, 0.870964, 0.758578, 0.575440, 0.251189]
absorbance, 2 cm cell  [0.048000, 0.120000, 0.240000, 0.480000, 1.200000]
```

The absorbance is linear in concentration: `0.024000` at {math}`2\ \mu`M and `0.600000` at {math}`50\ \mu`M, a factor of {math}`25` for a factor of {math}`25`. The transmittance is not linear: it falls from `0.946237` to `0.251189`, a loss of three quarters of the light. Doubling the path doubles every absorbance, `0.048000` to `1.200000`.

## A mixture at several wavelengths

Two absorbers X and Y have absorptivities at three wavelengths, one row for each wavelength. A mixture of {math}`3\times10^{-5}` mol/L of X and {math}`1\times10^{-5}` mol/L of Y in a {math}`1` cm cell is measured. The program computes the three absorbances and then solves the {math}`2\times2` system from the {math}`400` and {math}`600` nm readings to recover the concentrations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace sp = pluss::ma::natsci::chemistry::spectroscopy;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // two absorbers, X and Y, measured at three wavelengths: rows = wavelengths, absorptivities in L/(mol cm)
  const auto eps = mat({1.0e4, 2.0e3,    // 400 nm
                        4.0e3, 8.0e3,    // 500 nm
                        500.0, 1.5e4},   // 600 nm
                       3, 2);
  // a mixture of 3e-5 mol/L X and 1e-5 mol/L Y in a 1 cm cell: absorbance is additive
  const auto c = vec({3e-5, 1e-5});
  const auto a = sp::absorbance(eps, c, 1.0);
  out.writeln("absorbance at 400, 500, 600 nm  ", flat(a));
  // recover the concentrations from two of the wavelengths by solving the 2x2 linear system
  const auto e2 = mat({1.0e4, 2.0e3, 500.0, 1.5e4}, 2, 2);
  const auto a2 = vec({flat(a)[0], flat(a)[2]});
  const auto back = pluss::ma::natsci::common::dynamics::solve(e2, a2);
  out.writeln("recovered concentrations (mol/L) ", flat(back));
}
```

Output

```text
absorbance at 400, 500, 600 nm  [0.320000, 0.200000, 0.165000]
recovered concentrations (mol/L) [0.000030, 0.000010]
```

The mixture absorbs `0.320000`, `0.200000` and `0.165000` at the three wavelengths, the sums of the two contributions (at {math}`400` nm, {math}`0.30 + 0.02`). Two readings are enough to recover `0.000030` and `0.000010` mol/L exactly. With measurement noise a third wavelength would be used in a least-squares fit, and the wavelengths should be chosen where the two absorbers differ most, here {math}`400` nm for X and {math}`600` nm for Y.

## Decisions

- Use `absorbance` with a {math}`[\ldots, \text{species}]` layout whenever more than one absorber is present; the sum is done for you and a batch of samples stays one call.
- Work with absorbance, not transmittance, for concentrations: only absorbance is linear. Keep the absorbance below about {math}`1` to {math}`2` in practice, where the detector still sees light.
- Use a Series for the path length when cells differ between lanes, a number when they do not.

## Pitfalls

The species axis of the absorptivity and of the concentration must agree. The program passes one absorptivity against two concentrations, writes a single species correctly, and computes the absorbance of a strongly absorbing sample.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace sp = pluss::ma::natsci::chemistry::spectroscopy;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::vector<double> v, std::int64_t r, std::int64_t c) { return pluss::make_series(v, {r, c}).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // one absorptivity against two concentrations: the species axes disagree
  try {
    sp::absorbance(vec({1.0e4}), vec({3e-5, 1e-5}), 1.0);
  } catch (const pluss::Error &e) {
    out.writeln("mismatched species axes: ", e.message());
  }
  // a single species written with an explicit trailing axis is fine
  out.writeln("one species, one absorptivity: ", flat(sp::absorbance(vec({1.0e4}), mat({3e-5}, 1, 1), 1.0)));
  // a strongly absorbing sample: the law is exact but the transmitted light is below any detector's range
  const auto strong = sp::absorbance(vec({1.2e4}), mat({1e-3}, 1, 1), 1.0);
  out.writeln("absorbance ", flat(strong), " transmittance ", flat(sp::transmittance(strong)));
}
```

Output

```text
mismatched species axes: natsci: absorbance: absorptivity and concentration need the same number of species on the last axis
one species, one absorptivity: [0.3000]
absorbance [12.0000] transmittance [0.0000]
```

One absorptivity against two concentrations is an error that says what is wrong: the two species axes disagree, and the function does not guess that you meant the same absorptivity for both. A single species with an explicit trailing axis of size one is fine, `0.3000`. The last line shows a sample at {math}`1` mmol/L with absorbance `12.0000`: the law holds, but only {math}`10^{-12}` of the light arrives, which no instrument can read, and in practice such a sample is diluted.

## Where next

[Line shapes and broadening](line_shapes.md) describes what each absorption line looks like as a function of wavenumber, and [Diatomic partition functions](diatomic_thermodynamics.md) uses the spectroscopic constants of a molecule to compute its thermodynamics.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="line_shapes.html">
    <span class="label">Next</span>
    <span class="title">Line shapes and broadening</span>
  </a>
</div>
<!-- apiref:nav:end -->
