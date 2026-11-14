<!-- generated from atlas.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Atlas: stacks of equal tables

[Back to PlussMic by example](../guide.md)

An `Atlas` holds several `DataFrame`s of identical shape as one object: folds of a cross-validation, months of an export, scenarios of a model. This page uses it for all three. It is the least-used table type, and the programs show why: it is exactly right when the frames really are the same shape, and it refuses everything else.

## The mental model

An Atlas is a *strict cube*: every frame has the same number of rows and the same feature names in the same order, and the frames are addressed by a depth label (`loc("fold2")`) or a position. `map_frames` applies a table transformation to every frame and `aggregate_over_frames` sees all frames at once to produce one row per frame. If your pieces differ in size (train, validation and test splits, products that come and go), use an `Archive`, the Kindle model's version, on the [Kindle page](kindle_streams_archives.md).

## Cross-validation

Twelve noisy observations of a line, shuffled with a fixed seed and cut into four folds of three rows. Each fold takes a turn as the test set while a line is fitted on the other three.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::ScalarType;
using pluss::table::Atlas;
using pluss::table::DataFrame;
using pluss::table::Slate;

int main() {
  // Twelve observations y = 2 + 0.5 x with noise; shuffled by a fixed seed so folds mix early and late x values.
  const std::int64_t n = 12, folds = 4, per_fold = n / folds;
  auto x = pluss::series::arange(0, n, ScalarType::Float64);
  auto y = x.mul(0.5).add(2.0).add(pluss::series::randn({n}, ScalarType::Float64, 3).mul(0.4));
  auto order = pluss::series::randperm(n, ScalarType::Int64, 9);
  auto data = DataFrame({{"x", x.index_select(0, order)}, {"y", y.index_select(0, order)}});

  // 1. An Atlas needs frames of identical shape: four folds of three rows each.
  std::vector<DataFrame> parts;
  for (std::int64_t k = 0; k < folds; ++k) parts.push_back(data.slice(k * per_fold, (k + 1) * per_fold));
  Atlas atlas(parts, Slate(pluss::make_note({"fold0", "fold1", "fold2", "fold3"})));
  std::cout << "folds " << atlas.depth() << " of " << atlas.ncases() << " rows each\n";

  // 2. For each fold: fit y = a + b x on the other folds, predict the held-out fold, record the mean absolute error.
  std::vector<double> scores;
  for (std::int64_t k = 0; k < folds; ++k) {
    std::vector<pluss::Series> train_x, train_y;
    for (std::int64_t j = 0; j < folds; ++j) {
      if (j == k) continue;
      train_x.push_back(atlas.frame(j).features("x").as_series());
      train_y.push_back(atlas.frame(j).features("y").as_series());
    }
    auto tx = train_x[0].unite({train_x[1], train_x[2]}, 0);
    auto ty = train_y[0].unite({train_y[1], train_y[2]}, 0);
    auto design = pluss::series::ones({tx.numel(), 1}, ScalarType::Float64).unite({tx.unsqueeze(1)}, 1);
    auto beta = design.t().matmul(design).linearalg().inverse().matmul(design.t().matmul(ty.unsqueeze(1))).squeeze(1);
    auto hx = atlas.frame(k).features("x").as_series();
    auto hy = atlas.frame(k).features("y").as_series();
    auto prediction = hx.mul(beta.select(0, 1)).add(beta.select(0, 0));
    const double mae = prediction.sub(hy).abs().mean().item<double>();
    scores.push_back(mae);
    std::cout << "held out fold" << k << ": intercept " << beta.select(0, 0).item<double>() << ", slope " << beta.select(0, 1).item<double>() << ", MAE " << mae << "\n";
  }

  // 3. The cross-validated score is the mean of the fold errors; the spread says how much to trust it.
  auto s = pluss::make_series(scores);
  std::cout << "cross-validated MAE " << s.mean().item<double>() << " +/- " << s.std().item<double>() << " (noise level 0.4)\n";
}
```

Output

```text
folds 4 of 3 rows each
held out fold0: intercept 1.91928, slope 0.50932, MAE 0.46975
held out fold1: intercept 1.74762, slope 0.506927, MAE 0.347825
held out fold2: intercept 1.73872, slope 0.514216, MAE 0.509498
held out fold3: intercept 1.79317, slope 0.5068, MAE 0.254655
cross-validated MAE 0.395432 +/- 0.11636 (noise level 0.4)
```
The Atlas needs four frames of exactly three rows, which the program builds with `slice`. For each fold the fitted intercepts and slopes agree closely (about `1.7` to `1.9` and `0.51`), which is the first sign that the model is stable. The held-out errors range from `0.25` to `0.51` and average `0.395 +/- 0.116`, close to the noise level `0.4` that generated the data: the cross-validated error of a correct model is the noise.

## Monthly exports

Three months of a product table with the same schema. The program totals each month, derives revenue per unit, reads one month by label and then shows what a drifted schema does.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Atlas;
using pluss::table::DataFrame;
using pluss::table::Slate;

static DataFrame month(std::vector<std::string> products, std::vector<double> units, std::vector<double> revenue) {
  return DataFrame({{"product", pluss::make_note(products)}, {"units", pluss::make_series(units)}, {"revenue", pluss::make_series(revenue)}});
}

int main() {
  Atlas year({month({"widget", "gadget", "gizmo"}, {120, 80, 40}, {1200, 1600, 800}),
              month({"widget", "gadget", "gizmo"}, {150, 70, 55}, {1500, 1400, 1100}),
              month({"widget", "gadget", "gizmo"}, {90, 95, 60}, {900, 1900, 1200})},
             Slate(pluss::make_note({"2026-09", "2026-10", "2026-11"})));
  std::cout << "months " << year.depth() << ", cases per month " << year.ncases() << ", features";
  for (const auto &name : year.column_names()) std::cout << " " << name;
  std::cout << "\n";

  // 1. One summary row per month: total units and revenue. aggregate_over_frames sees every month at once.
  auto totals = year.aggregate_over_frames([](const std::vector<DataFrame> &frames) {
    std::vector<double> units, revenue;
    for (const auto &f : frames) {
      units.push_back(f.features("units").as_series().sum().item<double>());
      revenue.push_back(f.features("revenue").as_series().sum().item<double>());
    }
    return DataFrame({{"units", pluss::make_series(units)}, {"revenue", pluss::make_series(revenue)}});
  });
  std::cout << totals.to_string() << "\n";

  // 2. The revenue per unit by month, and the best month.
  auto per_unit = totals.assign({{"revenue_per_unit", [](const DataFrame &f) { return f.features("revenue") / f.features("units"); }}});
  std::cout << per_unit.to_string() << "\n";

  // 3. One month's table, by label, and the latest month with head/tail.
  std::cout << "October:\n" << year.loc("2026-10").to_string() << "\n";
  std::cout << "latest month only: depth " << year.tail(1).depth() << "\n";

  // 4. A month that arrives with a renamed column breaks the shared schema; the Atlas refuses it.
  try {
    auto drifted = month({"widget", "gadget", "gizmo"}, {100, 100, 100}, {1000, 1000, 1000}).rename("units", "units_sold");
    Atlas bad({year.frame(0), drifted});
    std::cout << "unexpectedly accepted\n";
  } catch (const pluss::Error &e) {
    std::cout << "rejected: " << e.message() << "\n";
  }
}
```

Output

```text
months 3, cases per month 3, features product units revenue
=====================
     | units  revenue
---------------------
0    | 240    3600   
1    | 275    4000   
2    | 245    4000   
---------------------
[3 cs x 2 feat]
=======================================
     | units  revenue  revenue_per_unit
---------------------------------------
0    | 240    3600     15              
1    | 275    4000     14.54545455     
2    | 245    4000     16.32653061     
---------------------------------------
[3 cs x 3 feat]
October:
==============================
     | product  units  revenue
------------------------------
0    | widget   150    1500   
1    | gadget   70     1400   
2    | gizmo    55     1100   
------------------------------
[3 cs x 3 feat]
latest month only: depth 1
rejected: Atlas: all frames must have identical column names/order
```
`aggregate_over_frames` is given all the months and returns one table with one row per month: units `240`, `275` and `245`, and revenue per unit of `15`, `14.5` and `16.3`. `loc("2026-10")` reads a month by its label and `tail(1)` keeps the last one. When the same month arrives with `units` renamed to `units_sold`, the Atlas is refused (`all frames must have identical column names/order`) rather than silently misaligning the columns.

## What-if scenarios

One product table under four assumptions: the baseline, prices up 10 %, costs up 15 % and both. `map_frames` computes profit in every scenario with the same formula.

```cpp
#include <pluss/pluss.h>

#include <iostream>

using pluss::table::Atlas;
using pluss::table::DataFrame;
using pluss::table::Slate;

static DataFrame scenario(double price_factor, double cost_factor) {
  const std::vector<double> price{20.0, 35.0, 50.0, 12.0}, cost{12.0, 25.0, 38.0, 9.0}, volume{500.0, 300.0, 120.0, 900.0};
  std::vector<double> p, c;
  for (std::size_t i = 0; i < price.size(); ++i) {
    p.push_back(price[i] * price_factor);
    c.push_back(cost[i] * cost_factor);
  }
  return DataFrame({{"product", pluss::make_note({"basic", "plus", "pro", "lite"})}, {"price", pluss::make_series(p)}, {"cost", pluss::make_series(c)}, {"volume", pluss::make_series(volume)}});
}

int main() {
  Atlas scenarios({scenario(1.0, 1.0), scenario(1.10, 1.0), scenario(1.0, 1.15), scenario(1.10, 1.15)},
                  Slate(pluss::make_note({"baseline", "price +10%", "cost +15%", "both"})));
  std::cout << "scenarios " << scenarios.depth() << ", products " << scenarios.ncases() << "\n";

  // 1. The same formula in every scenario: map_frames applies a DataFrame transform to each frame.
  auto with_profit = scenarios.map_frames([](const DataFrame &f) {
    return f.assign({{"profit", [](const DataFrame &g) { return (g.features("price") - g.features("cost")) * g.features("volume"); }}});
  });
  std::cout << "profit by product in the baseline:\n" << with_profit.loc("baseline").features({"product", "profit"}).to_string() << "\n";

  // 2. One row per scenario: total profit. aggregate_over_frames sees all scenarios at once.
  auto totals = with_profit.aggregate_over_frames([](const std::vector<DataFrame> &frames) {
    std::vector<double> total;
    for (const auto &f : frames) total.push_back(f.features("profit").as_series().sum().item<double>());
    return DataFrame({{"total_profit", pluss::make_series(total)}});
  });
  std::cout << totals.to_string() << "\n";

  // 3. Sensitivity against the baseline, in money and in percent.
  auto profit = totals.features("total_profit").as_series();
  const double base = profit.select(0, 0).item<double>();
  auto delta = profit.sub(base);
  std::cout << "change from baseline: " << pluss::series::print(delta) << "\n";
  std::cout << "percent of baseline:  " << pluss::series::print(delta.div(base).mul(100.0)) << "\n";

  // 4. Which product suffers most when costs rise? Compare product profits between the baseline and the cost scenario.
  auto drop = with_profit.loc("baseline").features("profit").as_series().sub(with_profit.loc("cost +15%").features("profit").as_series());
  std::cout << "profit lost per product when costs rise 15%: " << pluss::series::print(drop) << "\n";
}
```

Output

```text
scenarios 4, products 4
profit by product in the baseline:
======================
     | product  profit
----------------------
0    | basic    4000  
1    | plus     3000  
2    | pro      1440  
3    | lite     2700  
----------------------
[4 cs x 2 feat]
===================
     | total_profit
-------------------
0    | 11140       
1    | 14870       
2    | 7216        
3    | 10946       
-------------------
[4 cs x 1 feat]
change from baseline: Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 3730.0000, -3924.0000, -194.0000]
percent of baseline:  Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 33.4829, -35.2244, -1.7415]
profit lost per product when costs rise 15%: Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[900.0000, 1125.0000, 684.0000, 1215.0000]
```
The totals are `11140`, `14870`, `7216` and `10946`: against the baseline, a price rise adds `3730` (`+33.5 %`), a cost rise removes `3924` (`-35.2 %`) and both together cost `194`. The last line ranks the products by exposure to a cost rise: the lite and plus products lose the most, `1215` and `1125`. Four scenarios, one formula, one table of answers.

## Decisions

- **Equal shapes:** use an Atlas when frames share rows and features; use an Archive when they do not.
- **One formula, many frames:** `map_frames` rather than a loop, so every scenario is computed the same way.
- **Across frames:** `aggregate_over_frames` for anything that needs all frames at once.
- **Labels:** give the depth axis meaningful labels, such as the fold, the month or the scenario, and read frames by label.

## Where next

Reference pages: [`map_frames`](../api/atlas/map_frames.md), [`aggregate_over_frames`](../api/atlas/aggregate_over_frames.md), [`loc`](../api/atlas/loc.md), [`frame`](../api/atlas/frame.md), [`tail`](../api/atlas/tail.md), [`depth`](../api/atlas/depth.md), [`column_names`](../api/atlas/column_names.md). Next: [Catalogs](kindle_catalogs.md).
