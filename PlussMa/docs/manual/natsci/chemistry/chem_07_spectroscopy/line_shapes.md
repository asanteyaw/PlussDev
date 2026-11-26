# Line shapes and broadening

[Back to Chemistry 7: Spectroscopy](../chem_07_spectroscopy.md)

A spectral line is not a delta function: collisions and the motion of the molecules spread it. You want the shape of a line for fitting or simulation, the width that a mixture of broadening mechanisms gives, and the Doppler width at your temperature. This page evaluates the Gaussian, Lorentzian and Voigt profiles, checks their areas, and computes the Doppler broadening of a line.

## How it works

Three normalised profiles of the position {math}`x` around a line centre {math}`x_0` cover most work. The Gaussian, with standard deviation {math}`\sigma`, describes Doppler broadening, the Lorentzian, with half-width {math}`\gamma`, describes collisional and lifetime broadening, and the Voigt profile is their convolution:

```{math}
\begin{aligned}
&G(x) = \frac{e^{-u^2/2}}{\sigma\sqrt{2\pi}}, \\
&L(x) = \frac{\gamma/\pi}{(x - x_0)^2 + \gamma^2}, \quad V = G * L, \\
&u = \frac{x - x_0}{\sigma}
\end{aligned}
```

`gaussian(x, x0, sigma)`, `lorentzian(x, x0, gamma)` and `voigt(x, x0, sigma, gamma)` evaluate them, all with the same unit area. The Voigt profile is evaluated from the Faddeeva function `faddeeva(x, y)`, which is also available directly and returns the real and imaginary parts. `voigt_fwhm_approx(lorentz_fwhm, gauss_fwhm)` gives the full width at half maximum of the Voigt profile to within a fraction of a percent, and `doppler_sigma(nu0, T, molar_mass)` gives the Gaussian standard deviation of a line at {math}`\nu_0` for a gas of molar mass {math}`M` in kg/mol,

```{math}
\sigma_D = \frac{\nu_0}{c}\sqrt{\frac{k_B T N_A}{M}}
```

All arguments are Series of the same shape, so a spectrum is evaluated over a grid and many lines over many grids.

## The three profiles

The Gaussian with {math}`\sigma = 1`, the Lorentzian with {math}`\gamma = 1` and the Voigt profile with both are evaluated on a grid from {math}`-3` to {math}`3`, all centred on zero.

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
  // three line shapes with the same centre (0) and comparable widths, sampled from -3 to 3
  const auto x = vec({-3.0, -2.0, -1.0, 0.0, 1.0, 2.0, 3.0});
  const auto zero = vec({0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
  const auto one = zero.add(1.0);
  out.writeln("x                       ", flat(x));
  out.writeln("Gaussian  (sigma = 1)   ", flat(sp::gaussian(x, zero, one)));
  out.writeln("Lorentzian (gamma = 1)  ", flat(sp::lorentzian(x, zero, one)));
  out.writeln("Voigt (sigma = gamma = 1)", flat(sp::voigt(x, zero, one, one)));
}
```

Output

```text
x                       [-3.0000, -2.0000, -1.0000, 0.0000, 1.0000, 2.0000, 3.0000]
Gaussian  (sigma = 1)   [0.0044, 0.0540, 0.2420, 0.3989, 0.2420, 0.0540, 0.0044]
Lorentzian (gamma = 1)  [0.0318, 0.0637, 0.1592, 0.3183, 0.1592, 0.0637, 0.0318]
Voigt (sigma = gamma = 1)[0.0434, 0.0907, 0.1658, 0.2087, 0.1658, 0.0907, 0.0434]
```

The Gaussian has the highest peak, `0.3989`, and falls fastest: `0.0044` at three standard deviations. The Lorentzian peaks lower, `0.3183`, but has fat tails, `0.0318` at the same distance, seven times the Gaussian value. The Voigt profile lies between: `0.2087` at the centre, because the convolution spreads the weight, and `0.0434` at three, close to the Lorentzian tail.

## Areas, widths and Doppler broadening

A fine grid from {math}`-40` to {math}`40` checks that each profile integrates to one. The program then computes the approximate full width of the Voigt profile for a Gaussian of full width {math}`2.3548` (a unit {math}`\sigma`) and five Lorentzian widths, and the Doppler width of a {math}`6000` cm{math}`^{-1}` line of a {math}`28` g/mol gas at three temperatures.

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
  // area under each shape on a fine grid (should be 1) and the peak height
  std::vector<double> g;
  for (int i = -4000; i <= 4000; ++i) g.push_back(i * 0.01);
  const auto x = vec(g);
  const auto zero = x.mul(0.0);
  const auto one = zero.add(1.0);
  const double dx = 0.01;
  out.writeln("area: Gaussian, Lorentzian, Voigt  ", std::vector<double>{sp::gaussian(x, zero, one).sum().item<double>() * dx,
      sp::lorentzian(x, zero, one).sum().item<double>() * dx, sp::voigt(x, zero, one, one).sum().item<double>() * dx});
  // the Voigt full width against the common approximation, for several Lorentzian widths with a Gaussian FWHM of 2.355
  const auto lor = vec({0.0, 0.5, 1.0, 2.0, 5.0});
  const auto gau = lor.mul(0.0).add(2.3548);
  out.writeln("Lorentzian FWHM        ", flat(lor));
  out.writeln("approximate Voigt FWHM ", flat(sp::voigt_fwhm_approx(lor, gau)));
  // Doppler broadening of a line at 6000 cm^-1 for a molecule of 28 g/mol at three temperatures
  const auto nu0 = vec({6000.0, 6000.0, 6000.0});
  out.writeln("Doppler sigma (cm^-1)  ", flat(sp::doppler_sigma(nu0, vec({100.0, 300.0, 1000.0}), 0.028)));
}
```

Output

```text
area: Gaussian, Lorentzian, Voigt  [1.0000, 0.9841, 0.9841]
Lorentzian FWHM        [0.0000, 0.5000, 1.0000, 2.0000, 5.0000]
approximate Voigt FWHM [2.3548, 2.6336, 2.9350, 3.6013, 5.9836]
Doppler sigma (cm^-1)  [0.0034, 0.0060, 0.0109]
```

The Gaussian area is `1.0000`. The Lorentzian and Voigt areas are `0.9841`, not one: the grid stops at {math}`\pm40` and the tails beyond it hold {math}`2\gamma/(\pi\cdot40) = 1.6\%` of the weight, so the shortfall is the grid and not the function. With no Lorentzian width the full width is the Gaussian `2.3548`; at a Lorentzian width of `5` it is `5.9836`, close to the Lorentzian width alone. The Doppler width grows as {math}`\sqrt T`: `0.0034`, `0.0060` and `0.0109` cm{math}`^{-1}` at {math}`100`, {math}`300` and {math}`1000` K, a factor {math}`\sqrt{10}` between the first and the last.

## Decisions

- Use the Gaussian when Doppler broadening dominates (low pressure, high temperature), the Lorentzian when collisions or lifetime do (high pressure), and the Voigt profile for the crossover. The Voigt profile costs more but is the only one that is right in every regime.
- Normalise by the profile and not by its peak: all three integrate to one, so line strength multiplies them directly.
- Estimate a width with `voigt_fwhm_approx` before you build a grid, so the grid spacing is a fraction of the width, and so that it extends far enough in the wings.
- Pass the molar mass in kg/mol to `doppler_sigma`; the result has the units of `nu0`.

## Pitfalls

The Voigt profile needs a strictly positive Gaussian width and a non-negative Lorentzian one, and the area of a Lorentzian line cannot be found from a finite window. The program tries the two bad widths and integrates the Lorentzian over three windows.

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
  const auto x = vec({0.0, 1.0});
  // a zero Gaussian width and a negative Lorentzian width are rejected
  try {
    sp::voigt(x, vec({0.0, 0.0}), vec({0.0, 0.0}), vec({1.0, 1.0}));
  } catch (const pluss::Error &e) {
    out.writeln("zero sigma     : ", e.message());
  }
  try {
    sp::voigt(x, vec({0.0, 0.0}), vec({1.0, 1.0}), vec({-1.0, -1.0}));
  } catch (const pluss::Error &e) {
    out.writeln("negative gamma : ", e.message());
  }
  // the area of a Lorentzian line on a window of 10, 100 and 1000 half-widths: the missing tail is 2 / (pi * window)
  for (int half : {10, 100, 1000}) {
    std::vector<double> g;
    for (int i = -half * 10; i <= half * 10; ++i) g.push_back(i * 0.1);
    const auto xs = vec(g);
    const auto area = sp::lorentzian(xs, xs.mul(0.0), xs.mul(0.0).add(1.0)).sum().item<double>() * 0.1;
    out.writeln("window +-", std::to_string(half), " half-widths: area ", std::vector<double>{area});
  }
}
```

Output

```text
zero sigma     : natsci: voigt: sigma must be strictly positive
negative gamma : natsci: voigt: gamma must be non-negative
window +-10 half-widths: area [0.9369]
window +-100 half-widths: area [0.9936]
window +-1000 half-widths: area [0.9994]
```

The two bad widths are rejected with a message that names the function. The area is the quiet trap: a window of {math}`\pm10` half-widths holds only `0.9369` of the line, {math}`\pm100` holds `0.9936`, {math}`\pm1000` holds `0.9994`; the missing tail is {math}`2/(\pi\,\mathrm{window})` and falls only as one over the window. To normalise a fitted Lorentzian or Voigt line, use the analytic area of one, or a window of hundreds of widths.

## Where next

[Absorption and the Beer-Lambert law](absorption.md) puts line strength and concentration together, and [Diatomic partition functions](diatomic_thermodynamics.md) uses the same spectroscopic constants to compute thermodynamic properties.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="absorption.html">
    <span class="label">Previous</span>
    <span class="title">Absorption and the Beer-Lambert law</span>
  </a>
  <a class="next-link" href="diatomic_thermodynamics.html">
    <span class="label">Next</span>
    <span class="title">Diatomic partition functions</span>
  </a>
</div>
<!-- apiref:nav:end -->
