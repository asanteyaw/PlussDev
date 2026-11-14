<!-- generated from summaries_and_tables.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Summaries, rankings and tables

[Back to Series by example](../series.md)

Most analysis is a summary: who is best, how much per group, how does this year compare with that one. This page works three such questions end to end: ranking candidates, totalling sales by region and reading a year-by-month table.

## The mental model

Three families of operations do nearly all the work.

- **Order:** `argsort` gives the positions that would sort a series, `topk` the best few with their positions.
- **Reduce along an axis:** `sum(1)`, `mean(0)`, `argmax(1)` collapse one dimension; the dimension you name disappears from the shape.
- **Group:** `grouped_sum(keys, count)` and its relatives reduce the values that share a key, and `scatter_add` is the same idea written by hand.

Reshaping (`reshape`, `permute`, `select`) changes how the same numbers are indexed without copying them.

## Ranking candidates

Eight candidates with scores, two of them tied. The program orders them, finds the top three, computes everyone's rank, and compares with the average.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  auto score = pluss::make_series({71.5, 88.0, 64.2, 92.3, 79.9, 88.0, 55.1, 83.4});
  show("scores", score);

  // 1. argsort gives the positions that would sort the scores; descending puts the best first.
  auto order = score.argsort(0, true);
  show("candidates best first", order);

  // 2. topk returns the k largest values together with where they sit.
  auto [best, where] = score.topk_pair(3);
  show("top three scores", best);
  show("top three positions", where);

  // 3. The rank of each candidate (0 = best) is the argsort of the order: it is the inverse permutation.
  auto rank = order.argsort(0);
  show("rank of each candidate", rank);

  // 4. Which candidates are above average, and how far?
  const double mean = score.mean().item<double>();
  auto above = score.gt(mean);
  std::cout << "mean score " << mean << "; above average: " << above.sum().item<double>() << " of " << score.numel() << "\n";
  show("distance from the mean", score.sub(mean));

  // 5. A histogram of the scores in four equal bins between the lowest and highest score.
  show("histogram", score.histc(4));
}
```

Output

```text
scores: Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[71.5000, 88.0000, 64.2000, 92.3000, 79.9000, 88.0000, 55.1000, 83.4000]
candidates best first: Series(shape=[8], strides=[1], dtype=int64, platform=cpu, numel=8, contiguous=true)
data=[3, 1, 5, 7, 4, 0, 2, 6]
top three scores: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[92.3000, 88.0000, 88.0000]
top three positions: Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[3, 5, 1]
rank of each candidate: Series(shape=[8], strides=[1], dtype=int64, platform=cpu, numel=8, contiguous=true)
data=[5, 1, 6, 0, 4, 2, 7, 3]
mean score 77.8; above average: 5 of 8
distance from the mean: Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[-6.3000, 10.2000, -13.6000, 14.5000, 2.1000, 10.2000, -22.7000, 5.6000]
histogram: Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[2, 1, 1, 4]
```
`argsort(0, true)` gives the positions best first: candidate 3 (92.3), then 1 and 5 (both 88.0), and so on. The rank of every candidate is the argsort of that order, because sorting positions inverts the permutation: candidate 0 has rank 5, candidate 3 rank 0. `topk_pair(3)` returns values and positions together. Notice the tied pair: `argsort` lists 1 before 5 (a stable sort keeps tied elements in their original order) while `topk` lists 5 before 1. If ties matter to you, say so in your own tie-break rather than rely on either order. The histogram counts scores in four equal bins between the lowest and the highest: 2, 1, 1, 4.

## Totals by group

Ten sales in three regions, labelled by an integer key. The program computes count, total, mean and largest sale per region, each region's share of the whole, and each sale's share of its own region, then rebuilds the totals by hand with `scatter_add`.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  // Ten sales. region[i] is 0 = north, 1 = south, 2 = east for sale i.
  auto region = pluss::make_series({0, 1, 0, 2, 1, 0, 2, 2, 1, 0});
  auto amount = pluss::make_series({120.0, 80.0, 45.0, 200.0, 95.0, 60.0, 150.0, 30.0, 110.0, 75.0});
  const std::int64_t regions = 3;

  // 1. One reduction per region: the sum, mean and largest sale of each group.
  auto total = amount.grouped_sum(region, regions);
  auto mean = amount.grouped_mean(region, regions);
  auto largest = amount.grouped_max(region, regions);
  auto count = amount.grouped_count(region, regions);
  show("sales per region", count);
  show("total per region", total);
  show("mean per region", mean);
  show("largest per region", largest);

  // 2. Each region's share of the whole.
  show("share of total", total.div(total.sum()));

  // 3. Each sale as a fraction of its own region's total: gather the region total back onto every sale.
  auto own_total = total.index_select(0, region);
  show("sale / its region total", amount.div(own_total));

  // 4. scatter_add builds the same totals from first principles: add each amount into the slot named by its region.
  auto manual = pluss::series::zeros({regions}, pluss::ScalarType::Float64).scatter_add(0, region, amount);
  show("scatter_add totals", manual);
  std::cout << "grouped_sum and scatter_add agree: " << total.allclose(manual) << "\n";
}
```

Output

```text
sales per region: Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[4, 3, 3]
total per region: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[300.0000, 285.0000, 380.0000]
mean per region: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[75.0000, 95.0000, 126.6667]
largest per region: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[120.0000, 110.0000, 200.0000]
share of total: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0.3109, 0.2953, 0.3938]
sale / its region total: Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[0.4000, 0.2807, 0.1500, 0.5263, 0.3333, 0.2000, 0.3947, 0.0789, 0.3860, 0.2500]
scatter_add totals: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[300.0000, 285.0000, 380.0000]
grouped_sum and scatter_add agree: 1
```
The region totals are 300, 285 and 380, and the east is the largest at 39 % of the whole even though the north has the most sales (four). `index_select(0, region)` pushes each region's total back onto its sales, which turns a per-group number into a per-row one; that is the usual way to compute shares and deviations within groups. The last lines show that `grouped_sum` is the optimised form of "add every amount into the slot named by its region": `scatter_add` gives the same totals.

## A year-by-month table

Three years of monthly rainfall are stored flat. `reshape({3, 12})` lays them out as years by months without copying, after which every question is a reduction along an axis.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  // Three years of monthly rainfall (mm) laid out flat, January of year one first.
  auto flat = pluss::make_series({78.0, 62.0, 55.0, 48.0, 51.0, 40.0, 33.0, 38.0, 52.0, 71.0, 85.0, 90.0,
                                  82.0, 59.0, 61.0, 45.0, 49.0, 36.0, 30.0, 41.0, 58.0, 69.0, 80.0, 95.0,
                                  74.0, 66.0, 52.0, 50.0, 47.0, 42.0, 29.0, 35.0, 55.0, 73.0, 88.0, 92.0});
  // 1. reshape turns the flat series into a [year, month] table without copying the values.
  auto table = flat.reshape({3, 12});
  std::cout << "table is " << table.size(0) << " years by " << table.size(1) << " months\n";

  // 2. Reductions along an axis: dimension 1 runs across the months of one year.
  show("total per year", table.sum(1));
  show("wettest month (0 = Jan) per year", table.argmax(1));
  show("average per calendar month", table.mean(0));

  // 3. Running totals within each year, and the change from one month to the next.
  show("running total, year 1", table.cumsum(1).select(0, 0));
  show("month-on-month change, year 1", table.diff(1, 1).select(0, 0));

  // 4. Compare each year with the average year: positive means wetter than usual.
  show("departure from the average year", table.sub(table.mean(0)).sum(1));

  // 5. permute swaps the axes, so each row is now a calendar month and each column a year.
  show("March across the years", table.permute({1, 0}).select(0, 2));
}
```

Output

```text
table is 3 years by 12 months
total per year: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[703.0000, 705.0000, 703.0000]
wettest month (0 = Jan) per year: Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[11, 11, 11]
average per calendar month: Series(shape=[12], strides=[1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[78.0000, 62.3333, 56.0000, 47.6667, 49.0000, 39.3333, 30.6667, 38.0000, 55.0000, 71.0000, 84.3333, 92.3333]
running total, year 1: Series(shape=[12], strides=[1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[78.0000, 140.0000, 195.0000, 243.0000, 294.0000, 334.0000, 367.0000, 405.0000, 457.0000, 528.0000, 613.0000, 703.0000]
month-on-month change, year 1: Series(shape=[11], strides=[1], dtype=float64, platform=cpu, numel=11, contiguous=true)
data=[-16.0000, -7.0000, -7.0000, 3.0000, -11.0000, -7.0000, 5.0000, 14.0000, 19.0000, 14.0000, 5.0000]
departure from the average year: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-0.6667, 1.3333, -0.6667]
March across the years: Series(shape=[3], strides=[12], dtype=float64, platform=cpu, numel=3, contiguous=false)
data=[55.0000, 61.0000, 52.0000]
```
Dimension 1 runs across the months of a year, so `sum(1)` gives yearly totals (703, 705, 703 mm) and `argmax(1)` the wettest month of each year (11, December, three times). Dimension 0 runs down the years, so `mean(0)` is the average for each calendar month. `cumsum(1)` is the running total within a year and ends at the yearly total; `diff(1, 1)` is the change from one month to the next and is one element shorter. `permute({1, 0})` swaps the axes so that `select(0, 2)` is March across the years, and its `strides=[12]` in the output shows that this is a view onto the same numbers, not a copy.

## Decisions

- **One axis or all:** `sum()` reduces everything to one number; `sum(1)` keeps the other axes. Name the axis whenever the table has structure.
- **Rank or order:** `argsort` when you need positions, `topk` when you need only the best few.
- **Groups:** keys are integers `0 .. count - 1`; `grouped_*` reductions for the standard statistics, `scatter_add` and `scatter_reduce` for anything else.
- **Per-row values from per-group values:** `index_select(0, keys)`.

## Pitfalls

Shapes line up from the right. A column of shape `[3, 1]` and a row of shape `[4]` make a `3 x 4` table; a `[4]` and a `[3]` have nothing to align and are refused.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  auto column = pluss::make_series({1.0, 2.0, 3.0}).unsqueeze(1);   // shape [3, 1]
  auto row = pluss::make_series({10.0, 20.0, 30.0, 40.0});          // shape [4]
  show("column + row gives a 3 x 4 table", column.add(row));
  auto wrong = pluss::make_series({1.0, 2.0, 3.0});                 // shape [3]
  try {
    show("row + wrong", row.add(wrong));
  } catch (const pluss::Error &e) {
    std::cout << "error: " << e.message() << "\n";
  }
  // Forgetting unsqueeze: a [3] plus a [4] cannot align; the [3, 1] column is what makes it a table.
}
```

Output

```text
column + row gives a 3 x 4 table: Series(shape=[3,4], strides=[4,1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=
⎡ 11.0000 21.0000 31.0000 41.0000 ⎤
⎢ 12.0000 22.0000 32.0000 42.0000 ⎥
⎣ 13.0000 23.0000 33.0000 43.0000 ⎦
error: broadcast: incompatible shapes
```
If you meant a table, `unsqueeze(1)` the series that should become the column.

## Where next

Reference pages: [`argsort`](../../sphinx/api/series/argsort.md), [`topk_pair`](../../sphinx/api/series/topk_pair.md), [`histc`](../../sphinx/api/series/histc.md), [`grouped_sum`](../../sphinx/api/series/grouped_sum.md), [`grouped_mean`](../../sphinx/api/series/grouped_mean.md), [`grouped_max`](../../sphinx/api/series/grouped_max.md), [`grouped_count`](../../sphinx/api/series/grouped_count.md), [`scatter_add`](../../sphinx/api/series/scatter_add.md), [`index_select`](../../sphinx/api/series/index_select.md), [`reshape`](../../sphinx/api/series/reshape.md), [`permute`](../../sphinx/api/series/permute.md), [`cumsum`](../../sphinx/api/series/cumsum.md), [`diff`](../../sphinx/api/series/diff.md), [`argmax`](../../sphinx/api/series/argmax.md). Next: [Estimation and uncertainty](estimation.md).
