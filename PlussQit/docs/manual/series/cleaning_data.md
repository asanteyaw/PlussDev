<!-- generated from cleaning_data.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Cleaning and scaling data

[Back to Series by example](../series.md)

Data arrives with holes, spikes and columns on wildly different scales. This page takes two raw inputs, a sensor log and a feature table, and turns them into something you can trust: unusable values found and replaced, columns put on a common scale, outliers flagged. The programs are complete; every output below was printed by the program above it.

## The mental model

A `Series` is a rectangular block of numbers or booleans with a **shape** and a **dtype**. Every operation works on the whole block at once, so you describe *what* you want for all elements and never write the loop. The idioms of data cleaning are three:

- a **comparison** such as `lt`, `gt` or `isnan` produces a Boolean `Series` of the same shape, the **mask**;
- masks combine with `logical_or`, `logical_and` and `logical_not`;
- a mask then **selects** (`masked_select`, which returns only the chosen values), **replaces** (`where`, `masked_fill`) or **counts** (`sum`, because `true` counts as one).

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
  // x.where(condition, other) keeps x where the condition is false and takes `other` where it is true.
  auto repaired = log.where(unusable, centre);
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

Two details of the program matter in practice. `where` keeps the receiver where the condition is **false** and takes the replacement where it is true, so `log.where(unusable, centre)` reads "in `log`, put `centre` where it is unusable". And the comparison `log.gt(50.0)` on a NaN is false, which is why the missing values needed their own `isnan` test.

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
  auto scaled = features.sub(lo).div(hi.sub(lo));
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

`where` is the one that catches people. The program below asks four ways for "x where it is big, otherwise 0":

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
  auto big = x.gt(2.5);
  show("condition (x > 2.5)", big);
  // A common mistake: expecting "x where big, otherwise 0".
  show("x.where(big, 0)      ", x.where(big, 0.0));
  // What was meant: replace the values that are NOT big, i.e. take 0 where the condition is false.
  show("x.where(!big, 0)     ", x.where(big.logical_not(), 0.0));
  // masked_fill states the intent directly: fill with 0 where the mask is true.
  show("x.masked_fill(!big, 0)", x.masked_fill(big.logical_not(), 0.0));
  // With a Boolean receiver the order reads naturally: condition.where(a, b) picks a where it is true.
  show("big.where(x, 0)      ", big.where(x, pluss::make_series(0.0)));
}
```

Output

```text
condition (x > 2.5): Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, false, true, true]
x.where(big, 0)      : Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 2.0000, 0.0000, 0.0000]
x.where(!big, 0)     : Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 0.0000, 3.0000, 4.0000]
x.masked_fill(!big, 0): Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 0.0000, 3.0000, 4.0000]
big.where(x, 0)      : Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 0.0000, 3.0000, 4.0000]
```
`x.where(big, 0)` returned `[1, 2, 0, 0]`: it zeroed the values that were big, the opposite of the intention. Negate the condition (`x.where(big.logical_not(), 0)`), use `masked_fill`, which says what it does, or put the Boolean first, where `big.where(x, 0)` reads naturally.

## Where next

Reference pages: [`isnan`](../../sphinx/api/series/isnan.md), [`masked_select`](../../sphinx/api/series/masked_select.md), [`masked_fill`](../../sphinx/api/series/masked_fill.md), [`where`](../../sphinx/api/series/where.md), [`median`](../../sphinx/api/series/median.md), [`quantile`](../../sphinx/api/series/quantile.md), [`standardize`](../../sphinx/api/series/standardize.md), [`amin`](../../sphinx/api/series/amin.md), [`amax`](../../sphinx/api/series/amax.md), [`corrcoef`](../../sphinx/api/series/corrcoef.md). Next: [Summaries, rankings and tables](summaries_and_tables.md).
