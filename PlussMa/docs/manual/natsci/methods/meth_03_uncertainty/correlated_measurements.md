# Correlated measurements

[Back to Methods 3: Measurement and Uncertainty](../meth_03_uncertainty.md)

The parameters returned by a fit are correlated, and so are readings that share a calibration. You want the uncertainty of a quantity computed from them, and adding the variances of the inputs gets it wrong. This page carries a vector of values with its full covariance matrix through a linear map, applies that to a quantity computed from two fitted parameters, and shows how much the correlation changes the answer.

## How it works

A `CorrelatedMeasurement<D>` holds values {math}`x` of dimension {math}`D` with shape `[..., n]` and their covariance {math}`C` of dimension {math}`D^2` with shape `[..., n, n]`: the variances on the diagonal and the covariances off it. A linear map with a matrix {math}`A` transforms them as

```{math}
y = A x,\qquad C_y = A\,C\,A^{\top}
```

`map(A)` applies this with batched matrix products. A quantity {math}`g(x)` that is not linear is handled to first order with the delta method: take {math}`A` as the gradient of {math}`g` at the estimate, a `1 x n` matrix, and the variance of {math}`g` is the single entry of {math}`C_y`. The constructor checks that the covariance is symmetric with a non-negative diagonal, and, by `CovarianceCheck`, that it is positive semidefinite (the default), positive definite or only symmetric. The type is `pluss::ma::natsci::common::CorrelatedMeasurement<Dimension>`; `values()` and `covariance()` return the quantities, and the covariance has the dimension squared. The first program uses the estimates of [the fitting page](../meth_02_fitting/nonlinear_fits.md), {math}`(V_{\max}, K_m)`, whose covariance is {math}`s^2(J^{\top}J)^{-1}`; their units differ, so they are carried as plain numbers in the units in which they were fitted.

## The uncertainty of a derived parameter

The initial slope of the Michaelis-Menten curve is {math}`g = V_{\max}/K_m`, with gradient {math}`(1/K_m,\ -V_{\max}/K_m^2)`. The program builds the covariance of the estimates from the fit, maps it with the gradient, and compares with the result of ignoring the correlation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}
using Number = common::Dimensionless;   // Vmax and Km are carried as plain numbers, in the units they were fitted in
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  const auto fit = estimation::levenberg_marquardt(michaelis_menten, rate(), mat({{1.0, 1.0}}));
  // the covariance of the estimates: sigma^2 (J^T J)^{-1}, with sigma^2 = SSE / (n - p)
  const auto jtj = fit.jacobian.select(0, 0).transpose(0, 1).matmul(fit.jacobian.select(0, 0));
  const auto cov = jtj.linearalg.inverse().mul(flat(fit.sse)[0] / 4.0);
  out.writeln("estimates (Vmax, Km)      = ", flat(fit.beta));
  out.writeln("covariance                = ", flat(cov));
  const auto c = flat(cov);
  out.writeln("correlation of the estimates = ", c[1] / std::sqrt(c[0] * c[3]));
  const common::CorrelatedMeasurement<Number> estimate(common::SeriesQuantity<Number>::from_si(fit.beta.select(0, 0)), common::SeriesQuantity<Number>::from_si(cov));
  // the initial slope Vmax / Km: its gradient (1/Km, -Vmax/Km^2) is a 1 x 2 matrix, and map() applies  C_y = A C A^T
  const double vmax = flat(fit.beta)[0], km = flat(fit.beta)[1];
  const auto gradient = mat({{1.0 / km, -vmax / (km * km)}});
  const auto slope = estimate.map(gradient);
  out.writeln("initial slope Vmax / Km       = ", vmax / km);
  out.writeln("its standard uncertainty     = ", std::sqrt(flat(slope.covariance().si_value())[0]));
  // ignoring the correlation: only the diagonal of the covariance
  const auto naive = mat({{c[0], 0.0}, {0.0, c[3]}});
  const common::CorrelatedMeasurement<Number> independent(common::SeriesQuantity<Number>::from_si(fit.beta.select(0, 0)), common::SeriesQuantity<Number>::from_si(naive));
  out.writeln("treating them as independent = ", std::sqrt(flat(independent.map(gradient).covariance().si_value())[0]));
}
```

Output

```text
estimates (Vmax, Km)      = [1.91246, 2.74971]
covariance                = [0.00062, 0.00224, 0.00224, 0.01084]
correlation of the estimates = 0.86539
initial slope Vmax / Km       = 0.69551
its standard uncertainty     = 0.01905
treating them as independent = 0.02784
```

The estimates have the covariance `[0.00062, 0.00224; 0.00224, 0.01084]` and a correlation of `0.86539`: an error that raises {math}`V_{\max}` raises {math}`K_m` with it. The slope is `0.69551`, with a standard uncertainty of `0.01905`. Treating the two estimates as independent gives `0.02784`, almost one and a half times too large, because the ratio of two estimates that rise together hardly changes.

## Two readings from one ruler

Two lengths are read off the same ruler and share part of the error: {math}`a = 20` mm and {math}`b = 50` mm, with variances `1` and `4` mm² and covariance `0.5` mm². The program forms their sum, their difference and both together.

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
  using Cov = common::DimensionPower<common::Length, 2>;
  // two lengths read off the same ruler share an error: covariance [[1, 0.5], [0.5, 4]] mm^2 written in m^2
  const common::CorrelatedMeasurement<common::Length> pair(common::SeriesQuantity<common::Length>::from_si(pluss::series::make({0.020, 0.050})), common::SeriesQuantity<Cov>::from_si(pluss::series::make({{1.0e-6, 0.5e-6}, {0.5e-6, 4.0e-6}})));
  const auto sum = pair.map(pluss::series::make({{1.0, 1.0}}));
  const auto diff = pair.map(pluss::series::make({{1.0, -1.0}}));
  out.writeln("a + b = ", flat(sum.values().si_value()), " m, variance ", flat(sum.covariance().si_value().mul(1.0e6)), " mm^2  (independent: 5)");
  out.writeln("a - b = ", flat(diff.values().si_value()), " m, variance ", flat(diff.covariance().si_value().mul(1.0e6)), " mm^2  (independent: 5)");
  // a whole matrix at once: both combinations, and the full 2 x 2 covariance of the pair of results
  const auto both = pair.map(pluss::series::make({{1.0, 1.0}, {1.0, -1.0}}));
  out.writeln("(sum, difference) covariance = ", flat(both.covariance().si_value().mul(1.0e6)), " mm^2");
}
```

Output

```text
a + b = [0.0700] m, variance [6.0000] mm^2  (independent: 5)
a - b = [-0.0300] m, variance [4.0000] mm^2  (independent: 5)
(sum, difference) covariance = [6.0000, -3.0000, -3.0000, 4.0000] mm^2
```

The sum is `0.0700` m with variance `6.0000` mm², above the `5` that independent errors would give, since a shared error adds up. The difference is `-0.0300` m with variance `4.0000` mm², below `5`, since the shared part cancels. Mapped together, the covariance of the pair (sum, difference) is `[6, -3; -3, 4]` mm²: the two results are themselves correlated.

## A batch of covariances

The program holds three stations at once, values `[3, 2]` with a covariance matrix of shape `[3, 2, 2]` for each, checks them for positive definiteness and maps them to the mean of the two readings.

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
  using Cov = common::DimensionPower<common::Length, 2>;
  // three stations at once: values [3, 2] and covariance [3, 2, 2]; the map is applied to every station
  const auto values = common::SeriesQuantity<common::Length>::from_si(pluss::series::make(std::vector<double>{1, 2, 3, 4, 5, 6}, {3, 2}));
  const auto cov = common::SeriesQuantity<Cov>::from_si(pluss::series::make(std::vector<double>{1, 0.5, 0.5, 4,   2, 0, 0, 3,   1, -0.5, -0.5, 1}, {3, 2, 2}));
  const common::CorrelatedMeasurement<common::Length> stations(values, cov, common::CovarianceCheck::PositiveDefinite);
  const auto mean = stations.map(pluss::series::make({{0.5, 0.5}}));
  out.writeln("station means          = ", flat(mean.values().si_value()), "  shape ", mean.values().si_value().sizes());
  out.writeln("variance of each mean  = ", flat(mean.covariance().si_value()), "  shape ", mean.covariance().si_value().sizes());
  out.writeln("check level kept       = ", mean.check() == common::CovarianceCheck::PositiveSemidefinite ? "positive semidefinite" : "other");
}
```

Output

```text
station means          = [1.5000, 3.5000, 5.5000]  shape [3, 1]
variance of each mean  = [1.5000, 1.2500, 0.2500]  shape [3, 1, 1]
check level kept       = positive semidefinite
```

The means are `1.5000, 3.5000, 5.5000`, of shape `[3, 1]`, and the variances of the means `1.5000, 1.2500, 0.2500`, of shape `[3, 1, 1]`: one covariance per station. The mapped result is checked at the semidefinite level, not at the level the inputs were built with, since a map can produce a singular covariance from a regular one.

## Decisions

- Build the covariance from the fit, {math}`s^2(J^{\top}J)^{-1}`, rather than from the standard errors alone; the off-diagonal entries are what the correlation lives in.
- Use the default `PositiveSemidefinite` check for covariances that may be singular, for example of perfectly correlated readings, and `PositiveDefinite` when you will invert it.
- For a nonlinear quantity use the gradient as `A` and read the variance from the mapped covariance; for a quantity with a strongly curved dependence on the inputs, sample the distribution instead.
- Keep all components in the same dimension: a measurement of lengths cannot hold a length and a time. Convert to plain numbers, as above, for parameters of different units.

## Pitfalls

A matrix is only a covariance if it is symmetric and positive semidefinite. The program tries a correlation of `1.5`, an asymmetric matrix and a singular matrix under the strict check.

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
  using Cov = common::DimensionPower<common::Length, 2>;
  const auto values = common::SeriesQuantity<common::Length>::from_si(pluss::series::make({1.0, 2.0}));
  // a correlation above 1 is not a covariance: standard deviations 1 and 2 allow at most 2 off the diagonal
  try {
    const common::CorrelatedMeasurement<common::Length> bad(values, common::SeriesQuantity<Cov>::from_si(pluss::series::make({{1.0, 3.0}, {3.0, 4.0}})));
  } catch (const pluss::Error &e) {
    out.writeln("correlation 1.5:  ", e.message());
  }
  try {
    const common::CorrelatedMeasurement<common::Length> bad(values, common::SeriesQuantity<Cov>::from_si(pluss::series::make({{1.0, 2.0}, {0.0, 4.0}})));
  } catch (const pluss::Error &e) {
    out.writeln("asymmetric:       ", e.message());
  }
  try {
    const common::CorrelatedMeasurement<common::Length> bad(values, common::SeriesQuantity<Cov>::from_si(pluss::series::make({{1.0, 2.0}, {2.0, 4.0}})), common::CovarianceCheck::PositiveDefinite);
  } catch (const pluss::Error &e) {
    out.writeln("singular, strict: ", e.message());
  }
  const common::CorrelatedMeasurement<common::Length> fine(values, common::SeriesQuantity<Cov>::from_si(pluss::series::make({{1.0, 2.0}, {2.0, 4.0}})));
  out.writeln("singular, default check accepts the perfectly correlated pair.");
}
```

Output

```text
correlation 1.5:   natsci::common::CorrelatedMeasurement: covariance must be positive semidefinite
asymmetric:        natsci::common::CorrelatedMeasurement: covariance must be symmetric
singular, strict:  natsci::common::CorrelatedMeasurement: covariance must be positive definite
singular, default check accepts the perfectly correlated pair.
```

Each is refused with a message about the property it lacks: the first is not positive semidefinite, the second not symmetric, and the third not positive definite. The singular matrix of a perfectly correlated pair is accepted by the default check.

The diagonal holds variances, not standard deviations. The program builds the same pair twice, with the variance `0.09` and with the standard uncertainty `0.3` on the diagonal.

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
  using Cov = common::DimensionPower<common::Length, 2>;
  const auto values = common::SeriesQuantity<common::Length>::from_si(pluss::series::make({10.0, 20.0}));
  // the diagonal holds VARIANCES. A standard uncertainty of 0.3 m is 0.09 m^2 on the diagonal, not 0.3
  const auto row = pluss::series::make({{0.5, 0.5}});
  const common::CorrelatedMeasurement<common::Length> right(values, common::SeriesQuantity<Cov>::from_si(pluss::series::make({{0.09, 0.0}, {0.0, 0.09}})));
  const common::CorrelatedMeasurement<common::Length> wrong(values, common::SeriesQuantity<Cov>::from_si(pluss::series::make({{0.3, 0.0}, {0.0, 0.3}})));
  out.writeln("mean of the pair, standard uncertainty with 0.09 on the diagonal: ", std::sqrt(flat(right.map(row).covariance().si_value())[0]), " m");
  out.writeln("with 0.3 on the diagonal (std. dev. typed as variance):          ", std::sqrt(flat(wrong.map(row).covariance().si_value())[0]), " m");
}
```

Output

```text
mean of the pair, standard uncertainty with 0.09 on the diagonal: 0.2121 m
with 0.3 on the diagonal (std. dev. typed as variance):          0.3873 m
```

The mean of the pair has a standard uncertainty of `0.2121` m for the correct covariance and `0.3873` m for the wrong one, almost twice as large. Nothing in the numbers shows the mistake; square the standard uncertainties yourself.

`map` applies the linear map to the values as well. The program maps the estimates with the gradient of {math}`g = V_{\max}K_m`.

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
  using Cov = common::DimensionPower<common::Dimensionless, 2>;
  // map() is the LINEAR map y = A x. For a nonlinear quantity g(x), use g(x) as the value and take only the covariance from map(A = gradient of g)
  const double vmax = 1.9125, km = 2.7497;
  const common::CorrelatedMeasurement<common::Dimensionless> estimate(common::SeriesQuantity<common::Dimensionless>::from_si(pluss::series::make({vmax, km})),
                                                                       common::SeriesQuantity<Cov>::from_si(pluss::series::make({{0.00062, 0.00224}, {0.00224, 0.01084}})));
  const auto product = estimate.map(pluss::series::make({{km, vmax}}));   // gradient of g = Vmax * Km
  out.writeln("g(x) = Vmax Km                  = ", vmax * km);
  out.writeln("value returned by map(gradient) = ", flat(product.values().si_value()), "   (that is A x = 2 g here, not g)");
  out.writeln("variance from map(gradient)     = ", flat(product.covariance().si_value()), "   (this part is what the delta method needs)");
}
```

Output

```text
g(x) = Vmax Km                  = 5.2588
value returned by map(gradient) = [10.5176]   (that is A x = 2 g here, not g)
variance from map(gradient)     = [0.0679]   (this part is what the delta method needs)
```

The value returned by the map is `10.5176`, which is {math}`2g`, since {math}`A x` here is {math}`K_m V_{\max} + V_{\max}K_m`; the quantity is {math}`g = 5.2588`. Take the value from {math}`g(x)` itself and only the covariance, `0.0679`, from the map.

A matrix with the wrong number of columns, a non-finite value and a covariance of the wrong size are rejected.

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
  using Cov = common::DimensionPower<common::Length, 2>;
  const auto values = common::SeriesQuantity<common::Length>::from_si(pluss::series::make({1.0, 2.0}));
  const common::CorrelatedMeasurement<common::Length> ok(values, common::SeriesQuantity<Cov>::from_si(pluss::series::make({{1.0, 0.0}, {0.0, 1.0}})));
  try {
    (void)ok.map(pluss::series::make({{1.0, 1.0, 1.0}}));
  } catch (const pluss::Error &e) {
    out.writeln("matrix with three columns: ", e.message());
  }
  try {
    const common::CorrelatedMeasurement<common::Length> bad(common::SeriesQuantity<common::Length>::from_si(pluss::series::make({1.0, std::nan("")})), common::SeriesQuantity<Cov>::from_si(pluss::series::make({{1.0, 0.0}, {0.0, 1.0}})));
  } catch (const pluss::Error &e) {
    out.writeln("NaN value:                 ", e.message());
  }
  try {
    const common::CorrelatedMeasurement<common::Length> bad(values, common::SeriesQuantity<Cov>::from_si(pluss::series::make({{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}})));
  } catch (const pluss::Error &e) {
    out.writeln("3 x 3 covariance, 2 values: ", e.message());
  }
}
```

Output

```text
matrix with three columns:  natsci::common::CorrelatedMeasurement::map: matrix input width mismatch
NaN value:                  natsci::common::CorrelatedMeasurement: values must be finite
3 x 3 covariance, 2 values:  natsci::common::CorrelatedMeasurement: covariance width mismatch
```

Each error names the shape rule. A NaN value is refused in the constructor rather than passed into the map.

## Where next

[Measurements with an uncertainty](measurements.md) is the simpler case for independent errors. The covariance of fitted parameters comes from [Fitting a model to data](../meth_02_fitting/nonlinear_fits.md), and the identifiability of the parameters that make it nearly singular from [Sensitivities and identifiability](../meth_02_fitting/sensitivities.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
