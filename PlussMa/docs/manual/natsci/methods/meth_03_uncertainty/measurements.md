# Measurements with an uncertainty

[Back to Methods 3: Measurement and Uncertainty](../meth_03_uncertainty.md)

A measured value without its uncertainty is half a result. You want to carry the standard uncertainty of a reading through the arithmetic that turns readings into a result, in the right units, for one reading or for thousands at once. This page builds a measured quantity with a standard uncertainty, combines measurements by sums, differences, products and quotients, and keeps the result differentiable.

## How it works

A `Measurement` holds a value {math}`x` and a standard uncertainty {math}`u(x) \ge 0`, both with the same physical dimension. Sums and differences of independent measurements add the variances, and products and quotients add the relative variances, to first order:

```{math}
\begin{aligned}
&u(x \pm y) = \sqrt{u(x)^2 + u(y)^2}, \\
&\frac{u(xy)}{|xy|} = \sqrt{\left(\frac{u(x)}{x}\right)^2 + \left(\frac{u(y)}{y}\right)^2}
\end{aligned}
```

(the quotient has the same relative uncertainty as the product). The type is `pluss::ma::natsci::common::Measurement<Dimension, Rep>`, where `Rep` is `double` for a single reading or `pluss::Series` for a batch of any shape, and the dimension is tracked by the type: the product of a length and a time has the dimension of a length times a time, and `Measurement<Length> + Measurement<Time>` does not compile. `value()` and `standard_uncertainty()` return the quantities, `lower()` and `upper()` one standard uncertainty below and above, and `relative_standard_uncertainty()` the ratio {math}`u/|x|`. The programs measure the density of a block and a batch of lengths.

## From readings to a derived quantity

The density of a metal block is its mass divided by its volume, {math}`\rho = m/(abc)`. The program builds the mass and the three edges, each read once with its own standard uncertainty, forms the volume with the product operator and the density with the quotient operator, and prints the intervals and relative uncertainties.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace units = common::units;
using pluss::Series;
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  using Mass = common::ScalarQuantity<common::Mass>;
  using Length = common::ScalarQuantity<common::Length>;
  // a metal block: mass 0.4520 +- 0.0005 kg and three edges, each read once with its own standard uncertainty
  const common::Measurement<common::Mass> mass(Mass::from(0.4520, units::kilogram), Mass::from(0.0005, units::kilogram));
  const common::Measurement<common::Length> a(Length::from(0.0800, units::metre), Length::from(0.0002, units::metre));
  const common::Measurement<common::Length> b(Length::from(0.0500, units::metre), Length::from(0.0002, units::metre));
  const common::Measurement<common::Length> c(Length::from(0.0250, units::metre), Length::from(0.0002, units::metre));
  out.writeln("mass: ", mass.value().si_value(), " kg, relative uncertainty ", mass.relative_standard_uncertainty());
  out.writeln("edge a: ", a.value().si_value(), " m, interval [", a.lower().si_value(), ", ", a.upper().si_value(), "] m");
  const auto volume = a * b * c;
  out.writeln("volume  = ", volume.value().si_value() * 1.0e6, " +- ", volume.standard_uncertainty().si_value() * 1.0e6, " cm^3, relative ", volume.relative_standard_uncertainty());
  const auto density = mass / volume;
  out.writeln("density = ", density.value().si_value(), " +- ", density.standard_uncertainty().si_value(), " kg/m^3, relative ", density.relative_standard_uncertainty());
}
```

Output

```text
mass: 0.45200 kg, relative uncertainty 0.00111
edge a: 0.08000 m, interval [0.07980, 0.08020] m
volume  = 100.00000 +- 0.92871 cm^3, relative 0.00929
density = 4520.00000 +- 42.27437 kg/m^3, relative 0.00935
```

The mass has a relative uncertainty of `0.00111` and the interval of the edge {math}`a` is [`0.07980`, `0.08020`] m. The volume is {math}`100 \pm 0.9287` cm³ (`0.00929` relative) and the density {math}`4520 \pm 42.27` kg/m³ (`0.00935` relative). The thinnest edge dominates: its relative uncertainty is `0.008` against `0.0025` and `0.004` for the others, and the mass contributes almost nothing. The dimension of the result, a mass over a length cubed, is tracked by the type.

## A batch of readings

Measurements can hold a whole array. The program forms two batches of five lengths, each with its own uncertainty per reading, and prints the sum, the difference, the product and the relative uncertainties.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace units = common::units;
using pluss::Series;
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // five readings of a lab bench at once: a length measured with a different uncertainty at each position
  using Lengths = common::SeriesQuantity<common::Length>;
  const common::Measurement<common::Length, Series> a(Lengths::from_si(pluss::series::make({1.0, 2.0, 3.0, 4.0, 5.0})), Lengths::from_si(pluss::series::make({0.1, 0.1, 0.2, 0.2, 0.3})));
  const common::Measurement<common::Length, Series> b(Lengths::from_si(pluss::series::make({0.5, 0.5, 0.5, 0.5, 0.5})), Lengths::from_si(pluss::series::make({0.1, 0.2, 0.1, 0.2, 0.1})));
  const auto sum = a + b;
  const auto diff = a - b;
  out.writeln("a + b      = ", flat(sum.value().si_value()), " +- ", flat(sum.standard_uncertainty().si_value()));
  out.writeln("a - b      = ", flat(diff.value().si_value()), " +- ", flat(diff.standard_uncertainty().si_value()));
  const auto area = a * b;
  out.writeln("a * b      = ", flat(area.value().si_value()), " +- ", flat(area.standard_uncertainty().si_value()), " m^2");
  out.writeln("relative   = ", flat(a.relative_standard_uncertainty()));
}
```

Output

```text
a + b      = [1.5000, 2.5000, 3.5000, 4.5000, 5.5000] +- [0.1414, 0.2236, 0.2236, 0.2828, 0.3162]
a - b      = [0.5000, 1.5000, 2.5000, 3.5000, 4.5000] +- [0.1414, 0.2236, 0.2236, 0.2828, 0.3162]
a * b      = [0.5000, 1.0000, 1.5000, 2.0000, 2.5000] +- [0.1118, 0.4031, 0.3162, 0.8062, 0.5220] m^2
relative   = [0.1000, 0.0500, 0.0667, 0.0500, 0.0600]
```

The sum and the difference have the same uncertainties, `0.1414, 0.2236, 0.2236, 0.2828, 0.3162`, since variances add in both. The product is in m² with uncertainties `0.1118, 0.4031, 0.3162, 0.8062, 0.5220`, which depend on the sizes of the values as well as of the uncertainties. The relative uncertainties of the first batch are `0.1000, 0.0500, 0.0667, 0.0500, 0.0600`.

## Keeping the derivative

The values of a `Series` measurement stay on the autodiff tape. The program marks the raw readings, adds two measurements and asks for the derivative of the sum of the results with respect to the raw readings.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace units = common::units;
using pluss::Series;
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // the value stays on the autodiff tape: differentiate a propagated result with respect to the raw reading
  auto raw = pluss::series::make({3.0, 4.0});
  raw.tracks_slope_(true);
  using Lengths = common::SeriesQuantity<common::Length>;
  const common::Measurement<common::Length, Series> x(Lengths::from_si(raw), Lengths::from_si(pluss::series::make({0.3, 0.4})));
  const common::Measurement<common::Length, Series> y(Lengths::from_si(pluss::series::make({1.0, 2.0})), Lengths::from_si(pluss::series::make({0.4, 0.3})));
  const auto sum = x + y;
  out.writeln("sum = ", flat(sum.value().si_value()), " +- ", flat(sum.standard_uncertainty().si_value()));
  sum.value().si_value().sum().reverse();
  out.writeln("d(sum of values)/d(raw reading) = ", flat(raw.sensitivity()));
}
```

Output

```text
sum = [4.00000, 6.00000] +- [0.50000, 0.50000]
d(sum of values)/d(raw reading) = [1.00000, 1.00000]
```

The sum is {math}`(4, 6) \pm (0.5, 0.5)` and the derivative of the summed values with respect to each raw reading is `1.00000`, as it must be for a sum. The measurement did not cut the chain: you can differentiate a result, or a fit that uses it, with respect to the original readings.

## Decisions

- Use `double` for a few readings and `Series` for a column of them; the operators are the same, and the shapes of values and uncertainties must match.
- Give an uncertainty that is a standard uncertainty, one standard deviation, not a half-width of a range. For a rectangular range of half-width {math}`a`, the standard uncertainty is {math}`a/\sqrt 3`.
- Use these operators when the inputs are independent. When they are correlated, as two quantities from the same fit are, use `CorrelatedMeasurement` on the next page.
- The propagation is first order. A relative uncertainty above about ten percent, or a quotient whose denominator may reach zero, needs a sampling treatment instead.

## Pitfalls

The operators assume independent errors, and the same measurement used twice is not independent of itself. The program computes {math}`x - x` and {math}`x\,x` for {math}`x = 5.0 \pm 0.2` m.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace units = common::units;
using pluss::Series;
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the formulas assume INDEPENDENT errors. Subtracting a measurement from itself has exactly zero error, but the formula adds the variances
  const common::Measurement<common::Length> x(common::ScalarQuantity<common::Length>::from(5.0, units::metre), common::ScalarQuantity<common::Length>::from(0.2, units::metre));
  const auto zero = x - x;
  out.writeln("x - x  = ", zero.value().si_value(), " +- ", zero.standard_uncertainty().si_value(), " m   (the true value is exactly 0 +- 0)");
  const auto square = x * x;
  out.writeln("x * x  = ", square.value().si_value(), " +- ", square.standard_uncertainty().si_value(), " m^2   (the correct x^2 has 2 x u = ", 2.0 * 5.0 * 0.2, ")");
}
```

Output

```text
x - x  = 0.0000 +- 0.2828 m   (the true value is exactly 0 +- 0)
x * x  = 25.0000 +- 1.4142 m^2   (the correct x^2 has 2 x u = 2.0000)
```

{math}`x - x` is exactly zero, yet the operator reports `0.0000 ± 0.2828`, as if the two copies were different readings. {math}`x\,x` is reported with an uncertainty of `1.4142` where the correct uncertainty of {math}`x^2` is {math}`2xu = 2.0000`. Write {math}`x^2` as a single operation on the value, with the derivative {math}`2x`, and treat repeated use of one reading with `CorrelatedMeasurement`.

A measurement with a negative or non-finite uncertainty, a NaN value or mismatched shapes is rejected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace units = common::units;
using pluss::Series;
static std::vector<double> flat(const Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  using L = common::ScalarQuantity<common::Length>;
  try {
    const common::Measurement<common::Length> bad(L::from(5.0, units::metre), L::from(-0.2, units::metre));
  } catch (const pluss::Error &e) {
    out.writeln("negative uncertainty: ", e.message());
  }
  try {
    const common::Measurement<common::Length> bad(L::from(std::nan(""), units::metre), L::from(0.2, units::metre));
    out.writeln("NaN value accepted");
  } catch (const pluss::Error &e) {
    out.writeln("NaN value:            ", e.message());
  }
  try {
    const common::Measurement<common::Length> bad(L::from(5.0, units::metre), L::from(std::nan(""), units::metre));
  } catch (const pluss::Error &e) {
    out.writeln("NaN uncertainty:      ", e.message());
  }
  using Ls = common::SeriesQuantity<common::Length>;
  try {
    const common::Measurement<common::Length, Series> bad(Ls::from_si(pluss::series::make({1.0, 2.0, 3.0})), Ls::from_si(pluss::series::make({0.1, 0.1})));
  } catch (const pluss::Error &e) {
    out.writeln("length mismatch:      ", e.message());
  }
}
```

Output

```text
negative uncertainty:  natsci::common::Measurement: standard uncertainty must be non-negative
NaN value:             natsci::common::Measurement: value and standard uncertainty must be finite
NaN uncertainty:       natsci::common::Measurement: value and standard uncertainty must be finite
length mismatch:       natsci::common::Measurement: value and uncertainty shape mismatch
```

A negative uncertainty is refused and so is a NaN in the value or the uncertainty: a NaN would otherwise pass through the arithmetic and come out as a NaN result without a message. Batches whose value and uncertainty differ in shape are refused with `shape mismatch`.

## Where next

[Correlated measurements](correlated_measurements.md) propagates a full covariance matrix, which is what a fit returns, and [Fitting a model to data](../meth_02_fitting/nonlinear_fits.md) produces the estimates and intervals whose uncertainties you will often want to carry on. 

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="correlated_measurements.html">
    <span class="label">Next</span>
    <span class="title">Correlated measurements</span>
  </a>
</div>
<!-- apiref:nav:end -->
