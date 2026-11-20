<!-- generated from cleaning_data.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Cleaning and scaling data

[Back to Series by example](../series.md)

Data arrives with holes, spikes and columns on wildly different scales. This page takes two raw inputs, a sensor log and a feature table, and turns them into something you can trust: unusable values found and replaced, columns put on a common scale, outliers flagged. The programs are complete; every output below was printed by the program above it.

## How it works

A `Series` is a rectangular block of numbers or booleans with a **shape** and a **dtype**. Every operation works on the whole block at once, so you describe *what* you want for all elements and never write the loop. The idioms of data cleaning are three:

- a **comparison** such as `lt`, `gt` or `isnan` produces a Boolean `Series` of the same shape, the **mask**;
- masks combine with `logical_or`, `logical_and` and `logical_not`;
- a mask then **selects** (`masked_select`, which returns only the chosen values), **replaces** (`where`, `masked_fill`) or **counts** (`sum`, because `true` counts as one).

## Operators and methods

Arithmetic and comparison can be written with operators or with methods, and the two are the same operations: `x + y` is `x.add(y)`, `-x` is `x.neg()`, `x > 2` is `x.gt(2)`, and `+=` is `add_`. The guide uses both on purpose: operators for expressions that read like the formula on paper, methods where a call has more to say than an operator can (a power, a clamp, a reduction along an axis) or where it chains naturally with other methods. Mix them freely.

```cpp
#include <pluss/pluss.h>

#include <iostream>

int main() {
  auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  auto y = pluss::make_series({10.0, 20.0, 30.0, 40.0});

  // 1. Each operator has a method with the same result. allclose says the two Series agree.
  std::cout << "x + y   == x.add(y):   " << (x + y).allclose(x.add(y)) << "\n";
  std::cout << "x - y   == x.sub(y):   " << (x - y).allclose(x.sub(y)) << "\n";
  std::cout << "x * y   == x.mul(y):   " << (x * y).allclose(x.mul(y)) << "\n";
  std::cout << "y / x   == y.div(x):   " << (y / x).allclose(y.div(x)) << "\n";
  std::cout << "-x      == x.neg():    " << (-x).allclose(x.neg()) << "\n";

  // 2. A number may be on either side, and an expression reads the way it is written on paper.
  std::cout << "2 * x + 1   : " << pluss::series::print(2.0 * x + 1.0) << "\n";
  std::cout << "1 / x       : " << pluss::series::print(1.0 / x) << "\n";
  std::cout << "(y - x) / x : " << pluss::series::print((y - x) / x) << "\n";

  // 3. Comparisons give Boolean Series, with operators or methods.
  std::cout << "x > 2       : " << pluss::series::print(x > 2.0) << "\n";
  std::cout << "x.gt(2)     : " << pluss::series::print(x.gt(2.0)) << "\n";

  // 4. In place: += changes the Series it is applied to. The method with a trailing underscore does the same.
  auto a = x.clone();
  a += y;
  auto b = x.clone();
  b.add_(y);
  std::cout << "a += y      : " << pluss::series::print(a) << "\n";
  std::cout << "b.add_(y)   : " << pluss::series::print(b) << "\n";

  // 5. Methods take what operators cannot say: a power, a clamp, a fused multiply-add.
  std::cout << "x.pow(2)           : " << pluss::series::print(x.pow(2.0)) << "\n";
  std::cout << "y.clamp_max(25)    : " << pluss::series::print(y.clamp_max(25.0)) << "\n";
  std::cout << "x.addcmul(x, y, 0.1): " << pluss::series::print(x.addcmul(x, y, 0.1)) << "\n";
}
```

Output

```text
x + y   == x.add(y):   1
x - y   == x.sub(y):   1
x * y   == x.mul(y):   1
y / x   == y.div(x):   1
-x      == x.neg():    1
2 * x + 1   : Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[3.0000, 5.0000, 7.0000, 9.0000]
1 / x       : Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 0.5000, 0.3333, 0.2500]
(y - x) / x : Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[9.0000, 9.0000, 9.0000, 9.0000]
x > 2       : Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, false, true, true]
x.gt(2)     : Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, false, true, true]
a += y      : Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[11.0000, 22.0000, 33.0000, 44.0000]
b.add_(y)   : Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[11.0000, 22.0000, 33.0000, 44.0000]
x.pow(2)           : Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 4.0000, 9.0000, 16.0000]
y.clamp_max(25)    : Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10.0000, 20.0000, 25.0000, 25.0000]
x.addcmul(x, y, 0.1): Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.0000, 6.0000, 12.0000, 20.0000]
```
The first five lines show that each operator and its method agree. A number may be on either side (`2 * x + 1`, `1 / x`), and an expression such as `(y - x) / x` needs no intermediate names. A comparison gives a Boolean `Series` whichever way you write it. The in-place forms change the `Series` they are applied to: `a += y` and `b.add_(y)` both leave `[11, 22, 33, 44]`. The last block is what only the methods can do: `pow`, `clamp_max`, and `addcmul`, which computes `x + 0.1 * x * y` in one call. From here on the programs use whichever form reads best.

## A sensor log, start to finish

One reading a minute from a room thermometer. Two readings are missing, one is a spike of 85 degrees and one a sensor fault of -40. The program builds a mask of unusable readings, summarises the usable ones, replaces the unusable ones by their median, and reports.

```cpp
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>
#include <limits>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  const double missing = std::numeric_limits<double>::quiet_NaN();
  // One reading a minute from a room thermometer: two dropouts (NaN), a spike and a sensor fault.
  auto log = pluss::make_series({21.4, 21.6, missing, 22.0, 85.0, 22.3, 22.1, missing, 21.9, -40.0, 22.2, 22.4});
  show("raw log", log);

  // 1. A reading is unusable if it is missing or outside what the room can physically be.
  auto missing_mask = log.isnan();
  auto implausible = log.lt(0.0).logical_or(log.gt(50.0));
  auto unusable = missing_mask.logical_or(implausible);
  show("unusable", unusable);
  std::cout << "unusable readings: " << unusable.sum().item<double>() << " of " << log.numel() << "\n";

  // 2. Replace them with the median of the usable readings, so one fault cannot drag the summary.
  auto usable = log.masked_select(unusable.logical_not());
  show("usable", usable);
  const double centre = usable.median().item<double>();
  // pluss::series::where(condition, a, b) takes `a` where the condition is true and `b` elsewhere.
  auto repaired = pluss::series::where(unusable, centre, log);
  show("repaired", repaired);

  // 3. Report.
  std::cout << "centre (median of usable): " << centre << "\n";
  std::cout << "mean " << repaired.mean().item<double>() << "  std " << repaired.std().item<double>() << "\n";
  std::cout << "min " << repaired.min().item<double>() << "  max " << repaired.max().item<double>() << "\n";
  std::cout << "quartiles " << repaired.quantile(0.25).item<double>() << " " << repaired.quantile(0.5).item<double>() << " " << repaired.quantile(0.75).item<double>() << "\n";
}
```

Output

```text
raw log: Series(shape=[12], strides=[1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[21.4000, 21.6000, nan, 22.0000, 85.0000, 22.3000, 22.1000, nan, 21.9000, -40.0000, 22.2000, 22.4000]
unusable: Series(shape=[12], strides=[1], dtype=bool, platform=cpu, numel=12, contiguous=true)
data=[false, false, true, false, true, false, false, true, false, true, false, false]
unusable readings: 4 of 12
usable: Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[21.4000, 21.6000, 22.0000, 22.3000, 22.1000, 21.9000, 22.2000, 22.4000]
repaired: Series(shape=[12], strides=[1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[21.4000, 21.6000, 22.0000, 22.0000, 22.0000, 22.3000, 22.1000, 22.0000, 21.9000, 22.0000, 22.2000, 22.4000]
centre (median of usable): 22
mean 21.9917  std 0.274552
min 21.4  max 22.4
quartiles 21.975 22 22.125
```
Four of twelve readings are unusable: the two NaNs, the spike and the fault. The median of the eight usable readings is 22.0 and, unlike the mean, it would not have moved had the spike been 850. Replacing with it gives a mean of 21.99 and a standard deviation of 0.27, which describes the room; the raw log's mean would have been NaN, and with the NaNs removed it would still have been dragged to 22.09 by the spike and fault. The quartiles on the last line are the numbers you would put on a chart.

Two details of the program matter in practice. `pluss::series::where(unusable, centre, log)` reads "where it is unusable, take `centre`, otherwise keep `log`"; the method form keeps the receiver where the condition is true, so the same repair is `log.where(unusable.logical_not(), centre)`. And the comparison `log.gt(50.0)` on a NaN is false, which is why the missing values needed their own `isnan` test.

## Putting columns on one scale

A model that compares households should not be dominated by whichever column has the biggest numbers. This table has income in thousands, household size and floor area, one row per household, and one household with an income of 390.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  // Eight households: income (thousands), household size, and floor area (square metres). One row per household.
  auto features = pluss::make_series({{52.0, 3.0, 85.0},
                                      {61.0, 4.0, 110.0},
                                      {48.0, 2.0, 62.0},
                                      {75.0, 5.0, 140.0},
                                      {43.0, 1.0, 45.0},
                                      {58.0, 3.0, 95.0},
                                      {390.0, 4.0, 105.0},
                                      {66.0, 4.0, 120.0}});
  std::cout << "rows " << features.size(0) << ", columns " << features.size(1) << "\n";

  // 1. Standardise each column: subtract its mean and divide by its standard deviation (dimension 0 runs down the rows).
  auto z = features.standardize(0);
  show("z-scores", z);

  // 2. A value more than two standard deviations from its column mean is an outlier candidate.
  auto outlier = z.abs().gt(2.0);
  show("outlier flags per column", outlier.sum(0));
  show("row of the outlier", outlier.any(1).nonzero());

  // 3. Min-max scale to [0, 1], the other common choice when you must keep a bounded range.
  auto lo = features.amin(0, true);
  auto hi = features.amax(0, true);
  auto scaled = (features - lo) / (hi - lo);
  show("min-max scaled", scaled);

  // 4. Correlation between the columns (variables are rows of the transposed matrix).
  show("correlation of the columns", features.t().corrcoef());
}
```

Output

```text
rows 8, columns 3
z-scores: Series(shape=[8,3], strides=[3,1], dtype=float64, platform=cpu, numel=24, contiguous=true)
data=
⎡ -0.4271 -0.2085 -0.3552 ⎤
⎢ -0.3455  0.6255  0.5111 ⎥
⎢ -0.4633 -1.0426 -1.1521 ⎥
⎢ -0.2186  1.4596  1.5506 ⎥
⎢ -0.5086 -1.8766 -1.7411 ⎥
⎢ -0.3727 -0.2085 -0.0087 ⎥
⎢  2.6359  0.6255  0.3378 ⎥
⎣ -0.3002  0.6255  0.8576 ⎦
outlier flags per column: Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 0]
row of the outlier: Series(shape=[1,1], strides=[1,1], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=
[ 6 ]
min-max scaled: Series(shape=[8,3], strides=[3,1], dtype=float64, platform=cpu, numel=24, contiguous=true)
data=
⎡ 0.0259 0.5000 0.4211 ⎤
⎢ 0.0519 0.7500 0.6842 ⎥
⎢ 0.0144 0.2500 0.1789 ⎥
⎢ 0.0922 1.0000 1.0000 ⎥
⎢ 0.0000 0.0000 0.0000 ⎥
⎢ 0.0432 0.5000 0.5263 ⎥
⎢ 1.0000 0.7500 0.6316 ⎥
⎣ 0.0663 0.7500 0.7895 ⎦
correlation of the columns: Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=
⎡ 1.0000 0.3163 0.2115 ⎤
⎢ 0.3163 1.0000 0.9844 ⎥
⎣ 0.2115 0.9844 1.0000 ⎦
```
Dimension 0 runs down the rows, so `standardize(0)` and `amin(0, true)` work column by column. The z-scores flag the outlier: income is 2.6 standard deviations above its column mean in row 6, and no other column has a value beyond two. The second scaling shows why that matters. Min-max scaling squeezes the other seven incomes into `0` to `0.09` because the outlier took the whole top of the range, and standardising shows the same thing as z-scores of `-0.2` to `-0.5` for the seven against `2.6` for the outlier: an outlier is a reason to look at the data before choosing a scaler. The correlation matrix finishes the job: size and area move together (0.98) while income is only loosely related to either, partly because of that one household.

## Decisions

- **Which values are unusable:** write the rule as a mask, test it by counting (`mask.sum()`), and keep the mask. You will want it again to report what you changed.
- **Replace with what:** the median of the usable values is robust; the mean is not. For a trend use a moving statistic (see [Signals](signals.md)).
- **Which scaler:** `standardize` when the data has outliers or you will compare distances; min-max when a bounded range is a requirement and the data is already clean.
- **Which axis:** reductions and `standardize` take the dimension to run along; dimension 0 is down the rows (column statistics), dimension 1 is across (row statistics).

## Pitfalls

`where` has one mistake that actually happens: a condition that does not line up with the data. The program below shows the two equivalent spellings of "`x` where it is big, otherwise 0" and then a condition with the wrong length:

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  auto big = x.gt(2.5);
  // x.where(big, 0) keeps x where `big` is true and takes 0 elsewhere; the free form writes the condition first.
  show("x.where(big, 0)              ", x.where(big, 0.0));
  show("pluss::series::where(big, x, 0)", pluss::series::where(big, x, 0.0));

  // A condition with five flags for four values cannot line up with the data.
  try {
    auto flags = pluss::make_series({true, false, true, false, true});
    show("never printed", x.where(flags, 0.0));
  } catch (const pluss::Error &e) {
    std::cout << "wrong length: " << e.message() << "\n";
  }
}
```

Output

```text
x.where(big, 0)              : Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 0.0000, 3.0000, 4.0000]
pluss::series::where(big, x, 0): Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 0.0000, 3.0000, 4.0000]
wrong length: broadcast: incompatible shapes
```
Both spellings return `[0, 0, 3, 4]`. The five flags against four values cannot broadcast, so the call fails with a broadcast error instead of guessing: build the condition from the data it is meant for (`x.gt(2.5)`) rather than typing it separately.

## Where next

Reference pages: [`add`](../../sphinx/api/series/add.md), [`sub`](../../sphinx/api/series/sub.md), [`mul`](../../sphinx/api/series/mul.md), [`div`](../../sphinx/api/series/div.md), [`neg`](../../sphinx/api/series/neg.md), [`gt`](../../sphinx/api/series/gt.md), [`pow`](../../sphinx/api/series/pow.md), [`clamp_max`](../../sphinx/api/series/clamp_max.md), [`addcmul`](../../sphinx/api/series/addcmul.md), [`operators`](../../sphinx/api/series/operators.md), [`isnan`](../../sphinx/api/series/isnan.md), [`masked_select`](../../sphinx/api/series/masked_select.md), [`masked_fill`](../../sphinx/api/series/masked_fill.md), [`where`](../../sphinx/api/series/where.md), [`median`](../../sphinx/api/series/median.md), [`quantile`](../../sphinx/api/series/quantile.md), [`standardize`](../../sphinx/api/series/standardize.md), [`amin`](../../sphinx/api/series/amin.md), [`amax`](../../sphinx/api/series/amax.md), [`corrcoef`](../../sphinx/api/series/corrcoef.md). Next: [Summaries, rankings and tables](summaries_and_tables.md).
