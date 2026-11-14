<!-- generated from estimation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Estimation and uncertainty

[Back to Series by example](../series.md)

An estimate without an error bar is half a result. This page fits a line to noisy data with the linear-algebra accessor and reports how uncertain the coefficients are, then quantifies the uncertainty of an average by resampling, and ends with the numeric pitfalls that make estimates quietly wrong.

## The mental model

Everything is array arithmetic. A regression is a few matrix products; a bootstrap is a table in which each row is one resample, reduced along its axis. Random numbers come from factory functions that take a **seed**, so a result can be reproduced exactly. Matrix routines live behind an accessor: `x.linearalg().inverse()`.

## Fitting a line

Two hundred points scatter around `y = 3 + 2x` with noise of standard deviation 1.5. The program builds the design matrix, solves the normal equations, and then checks the fit.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  const std::int64_t n = 200;
  // Synthetic truth: y = 3 + 2 x plus noise of standard deviation 1.5. A fixed seed makes the run repeatable.
  auto x = pluss::series::linspace(0.0, 10.0, n, ScalarType::Float64);
  auto noise = pluss::series::randn({n}, ScalarType::Float64, 7).mul(1.5);
  auto y = x.mul(2.0).add(3.0).add(noise);

  // 1. Design matrix [1, x]: a column of ones for the intercept next to the column of x. Shape [n, 2].
  auto design = pluss::series::ones({n, 1}, ScalarType::Float64).unite({x.unsqueeze(1)}, 1);
  std::cout << "design matrix shape [" << design.size(0) << ", " << design.size(1) << "]\n";

  // 2. Normal equations: beta = (X'X)^-1 X'y.
  auto xtx_inv = design.t().matmul(design).linearalg().inverse();
  auto beta = xtx_inv.matmul(design.t().matmul(y.unsqueeze(1)));
  show("beta (intercept, slope)", beta.squeeze(1));

  // 3. Residuals, noise level and standard errors of the coefficients.
  auto residual = y.unsqueeze(1).sub(design.matmul(beta));
  const double dof = static_cast<double>(n - 2);
  const double sigma2 = residual.square().sum().item<double>() / dof;
  std::cout << "residual standard deviation " << std::sqrt(sigma2) << " (true 1.5)\n";
  show("standard errors", xtx_inv.diagonal().mul(sigma2).sqrt());

  // 4. R squared: the share of the variance of y that the line explains.
  const double ss_res = residual.square().sum().item<double>();
  const double ss_tot = y.sub(y.mean()).square().sum().item<double>();
  std::cout << "R squared " << 1.0 - ss_res / ss_tot << "\n";

  // 5. Predict at new points.
  auto fresh = pluss::make_series({0.0, 5.0, 12.0});
  auto newx = pluss::series::ones({3, 1}, ScalarType::Float64).unite({fresh.unsqueeze(1)}, 1);
  show("prediction at x = 0, 5, 12", newx.matmul(beta).squeeze(1));
}
```

Output

```text
design matrix shape [200, 2]
beta (intercept, slope): Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[3.3635, 1.9405]
residual standard deviation 1.41727 (true 1.5)
standard errors: Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.1997, 0.0345]
R squared 0.940965
prediction at x = 0, 5, 12: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3.3635, 13.0661, 26.6499]
```
The fitted intercept and slope are `3.36` and `1.94`, within two standard errors of the true `3` and `2` (the standard errors are `0.20` and `0.035`). The residual standard deviation of `1.42` is close to the `1.5` that generated the noise, and R squared is `0.94`: the line explains 94 % of the variance of `y`. The design matrix has a column of ones next to the column of `x`; `unite` places the two side by side along dimension 1. The last line predicts at new points with the same matrix product.

## How sure are we of an average?

Fifty delivery times, skewed to the right. The mean tells you a number; the **bootstrap** tells you how much to believe it. The program draws 2000 resamples at once and reduces each.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Fifty delivery times in minutes, skewed to the right as delivery times are.
  auto times = pluss::series::randn({50}, ScalarType::Float64, 11).mul(0.5).exp().mul(20.0);
  std::cout << "sample mean " << times.mean().item<double>() << ", sample median " << times.median().item<double>() << "\n";

  // 1. 2000 resamples at once: each row of `pick` is one resample of 50 positions drawn with replacement.
  const std::int64_t resamples = 2000, n = times.numel();
  auto pick = pluss::series::randint(0, n, {resamples, n}, ScalarType::Int64, 5);
  auto draws = times.index_select(0, pick.reshape({resamples * n})).reshape({resamples, n});
  std::cout << "resample matrix " << draws.size(0) << " x " << draws.size(1) << "\n";

  // 2. One statistic per resample: reduce along the resample axis.
  auto means = draws.mean(1);
  auto medians = draws.median(1);

  // 3. The spread of the statistic across resamples is its uncertainty; the 2.5% and 97.5% quantiles give a 95% interval.
  std::cout << "bootstrap standard error of the mean " << means.std().item<double>() << "\n";
  std::cout << "95% interval for the mean   [" << means.quantile(0.025).item<double>() << ", " << means.quantile(0.975).item<double>() << "]\n";
  std::cout << "95% interval for the median [" << medians.quantile(0.025).item<double>() << ", " << medians.quantile(0.975).item<double>() << "]\n";
}
```

Output

```text
sample mean 22.4702, sample median 19.7271
resample matrix 2000 x 50
bootstrap standard error of the mean 1.43961
95% interval for the mean   [19.7714, 25.3117]
95% interval for the median [17.329, 22.9803]
```
Each row of `pick` is one resample of positions drawn with replacement, `index_select` turns the positions into values, and `reshape` lays the resamples out as a `2000 x 50` table. The mean of each row gives 2000 means; their standard deviation, `1.44`, is the standard error of the mean, and the 2.5 % and 97.5 % quantiles give a 95 % interval of `[19.8, 25.3]`. The median, which is robust to the long right tail, has a lower and slightly wider interval `[17.3, 23.0]`. All of it is one table of random numbers and a few reductions, with no loop over resamples.

## Decisions

- **Reproducibility:** pass a seed to every random factory (`randn`, `randint`, `rand`) in a program you want to repeat.
- **Direct inverse or a solver:** the normal equations are fine for a handful of well-scaled columns; for ill-conditioned problems use the accessor's `pinverse` or `qr` routines.
- **Parametric or bootstrap:** use the standard error formulas when the model assumptions hold; use the bootstrap when they do not or when the statistic has no formula (a median, a ratio).
- **Precision:** compute in `Float64` unless you have a reason not to (below).

## Pitfalls

Two surprises concern the host boundary and precision. `item<T>()` reads one value from a rank-zero `Series`; given a vector it refuses and names the right call.

```cpp
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  auto x = pluss::make_series({1.0, 2.0, 3.0});
  std::cout << "mean as a number: " << x.mean().item<double>() << "\n";
  try {
    std::cout << x.item<double>() << "\n";
  } catch (const pluss::Error &e) {
    std::cout << "error: " << e.message() << "\n";
  }
  std::vector<double> values = x.items<double>();
  std::cout << "all three values through items<double>: " << values[0] << " " << values[1] << " " << values[2] << "\n";
  std::cout << "one element through select: " << x.select(0, 1).item<double>() << "\n";
}
```

Output

```text
mean as a number: 2
error: Series::item<T>(): expected a rank-zero Series, got a rank-1 Series (shape [3]). Use items<T>() to read a rank-one Series as a std::vector<T>.
all three values through items<double>: 1 2 3
one element through select: 2
```
And the dtype carries the precision. A million readings of `0.1` summed in `Float32` drift by 22 units, and the mean of integers is a `Float32`, which cannot hold `100000001.5`.

```cpp
#include <pluss/pluss.h>

#include <iomanip>
#include <iostream>

int main() {
  using pluss::ScalarType;
  std::cout << std::setprecision(12);
  // A million readings of 0.1. Float32 keeps about seven digits, so the running total drifts; Float64 keeps about sixteen.
  auto exact = pluss::series::full({1000000}, 0.1, ScalarType::Float64);
  std::cout << "sum in Float64: " << exact.sum().item<double>() << "\n";
  std::cout << "sum in Float32: " << exact.to(ScalarType::Float32).sum().item<double>() << "\n";

  // The mean of an integer Series is a floating-point Series, and it is Float32 unless you convert first.
  auto ids = pluss::make_series({100000001, 100000002});
  auto narrow = ids.mean();
  auto wide = ids.to(ScalarType::Float64).mean();
  std::cout << "mean of integers is " << narrow.scalar_type() << ": " << narrow.item<double>() << "\n";
  std::cout << "after to(Float64) it is " << wide.scalar_type() << ": " << wide.item<double>() << "\n";
  std::cout << "the true mean is 100000001.5\n";
}
```

Output

```text
sum in Float64: 99999.9999999
sum in Float32: 100022.351562
mean of integers is float32: 100000000
after to(Float64) it is float64: 100000001.5
the true mean is 100000001.5
```
Convert with `to(ScalarType::Float64)` before reducing whenever the answer needs more than about seven digits.

## Where next

Reference pages: [`linearalg`](../../sphinx/api/series/linearalg.md), [`randn`](../../sphinx/api/series_factory/randn.md), [`randint`](../../sphinx/api/series_factory/randint.md), [`unite`](../../sphinx/api/series/unite.md), [`unsqueeze`](../../sphinx/api/series/unsqueeze.md), [`index_select`](../../sphinx/api/series/index_select.md), [`quantile`](../../sphinx/api/series/quantile.md), [`median`](../../sphinx/api/series/median.md), [`std`](../../sphinx/api/series/std.md), [`to`](../../sphinx/api/series/to.md). Next: [Simulation](simulation.md).
