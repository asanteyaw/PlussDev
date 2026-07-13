# Data Workflows

This guide teaches the day-to-day `DataFrame`, `Thread`, and `Atlas` workflow.
It is usage-first: load data, inspect it, select columns, filter rows, derive
features, aggregate, plot, and save results.

## Includes

For most applications:

```cpp
#include <pluss/pluss.h>
```

For loader option structs and utility helpers:

```cpp
#include <pluss/tils.h>
```

## Load a CSV

```cpp
using pluss::table::DataFrame;

auto df = DataFrame::load_csv("data/bank-full.csv", {
    {"ncases", 1000},
    {"features", {"age", "job", "balance", "duration"}}
});

df.head(8);
df.info();
df.describe();
```

Use typed options when you want compile-time discoverability:

```cpp
pluss::loader::ReadCsvOptions options;
options.sep = ';';
options.ncases = 1000;
options.usefeats = {"age", "job", "balance", "duration"};
options.case_col = "job";

auto df = DataFrame::load_csv("data/bank-full.csv", options);
```

## Load Excel

```cpp
auto prices = DataFrame::load_excel("report.xlsx", "Prices");
auto first_sheet = DataFrame::load_excel("report.xlsx", 0);
```

Load all workbook sheets when you need sheet names:

```cpp
auto workbook = DataFrame::load_excel("report.xlsx", nullptr, true);
for (const auto& sheet : workbook.sheet_names()) {
  workbook.sheets(sheet).head();
}
```

See [Loader Guide](loader.md) for CSV, TXT, Excel, JSON, Parquet, URLs, bad-line
policies, selected features, and case-column options.

## Inspect shape and schema

```cpp
std::int64_t nrows = df.ncases();
std::int64_t ncols = df.nfeatures();
const auto& names = df.features();

df.head(5);
df.tail(5);
df.view(10, 20);
df.info();
df.describe();
```

## Select features

```cpp
auto age = df.features("age");
auto numeric = df.features({"age", "balance", "duration"});
auto only_series = df.series_features();
auto only_text = df.note_features();
```

`features("name")` returns a `Thread`. `features({"a", "b"})` returns a new
`DataFrame` with the selected features.

## Work with a Thread

`Thread` is the column abstraction. Numeric threads support numeric transforms
and reductions; text threads support text transforms; time threads support
calendar transforms.

```cpp
auto balance = df.features("balance");

auto positive = balance.clip(0.0, 1'000'000.0);
auto mean_balance = balance.mean().item<double>();
auto missing_mask = balance.isnull();
```

Access the underlying payload only when you need lower-level APIs:

```cpp
auto balance_series = df.features("balance").as_series();
auto values = balance_series.to_vec<double>();
```

## Filter cases

Use `query` for readable table expressions:

```cpp
auto older = df.query("age >= 45");
auto older_known = df.query("age >= 45 and job != 'unknown'");
auto band = df.query("age between 45 and 50");
auto selected_jobs = df.query("job in ('management', 'technician')");
```

Use an explicit boolean `Series` mask when the predicate comes from C++ logic:

```cpp
auto ages = df.features("age").as_series().to_vec<std::int64_t>();
auto mask = pluss::Series::zeros(
    {static_cast<std::int64_t>(ages.size())},
    pluss::codem::ScalarType::Bool);

auto* keep = reinterpret_cast<bool*>(mask.raw_data());
for (std::size_t i = 0; i < ages.size(); ++i) {
  keep[i] = ages[i] >= 45;
}

auto filtered = df.filter(mask);
```

## Derive features

Use `assign` when new features depend on existing features:

```cpp
auto enriched = df.assign({{
    "balance_per_second",
    [](const DataFrame& frame) {
      return frame.features("balance") / frame.features("duration");
    },
}});
```

Use `transform` when you are changing existing features:

```cpp
auto adjusted = df.transform({
    {"age", [](const pluss::table::Thread& feature) { return feature + 1.0; }},
    {"balance", [](const pluss::table::Thread& feature) { return feature.clip(-1000.0, 1000.0); }},
});
```

For a single feature, `set_features` is concise:

```cpp
auto doubled = df.set_features("balance_x2", df.features("balance") * 2.0);
```

## Sort and take rows

```cpp
auto by_age = df.sort_values({"age"});
auto by_age_then_duration = df.sort_values({
    {"age", true, true},
    {"duration", false, true},
});

auto top_balance = df.top("balance", 10);
auto bottom_balance = df.bottom("balance", 10);
auto sample = df.sample(100, 1234);
```

## Missing data

```cpp
auto missing = df.isna();
auto filled = df.fillna(0.0, {"balance", "duration"});
auto forward = df.ffill();
auto backward = df.bfill();
auto complete = df.dropna();
```

`Thread` has the same style for column-level workflows:

```cpp
auto clean_balance = df.features("balance").fillna(0.0).clip(0.0, 1'000'000.0);
```

## Aggregate

Use simple reductions for numeric summaries:

```cpp
df.features({"age", "balance", "duration"}).mean(true);
df.features({"age", "balance", "duration"}).corr(true);
```

Use `aggregate` for named summary outputs:

```cpp
df.aggregate(
    {
        {"cases", "age", DataFrame::SummaryOp::Count},
        {"age_mean", "age", DataFrame::SummaryOp::Mean},
        {"job_cardinality", "job", DataFrame::SummaryOp::Nunique},
    },
    true);
```

Use `group_by` for grouped summaries:

```cpp
df.group_by(
    {"job"},
    {
        {"mean_balance", "balance", DataFrame::GroupOp::Mean},
        {"max_duration", "duration", DataFrame::GroupOp::Max},
    },
    true,
    true);
```

Convenience helpers are available for common group summaries:

```cpp
df.count_by("job", 10, true, true);
df.mean_by("job", "balance", 10, 1, true, true);
```

## Time-series table operations

For date/time features, use the time accessors and resampling helpers:

```cpp
auto daily = df.resample("date", {"close", "volume"}, "D", "mean", 1, true);
auto lagged = df.shift({"close"}, 1);
auto rolling = df.rolling({
    {"close_5_mean", "close", pluss::table::ThreadWindowOp::RollingMean, 5, 3},
});
```

For column-level time-series math:

```cpp
auto returns = df.features("close").ts().returns(1);
auto rolling_std = df.features("close").ts().rolling_std(20, 10);
```

See [Time Series Accessor Guide](time_series_accessor_guide.md) for more.

## Plot

Use DataFrame plot accessors for table-shaped plots and PlussPl for explicit
plot construction.

```cpp
auto prices = df.features({"date", "close", "volume"});
prices.plot();
```

For manual plots:

```cpp
#include <pluss/pl.h>

namespace pl = pluss::pl;

auto fig = pl::studio("balance");
fig.pane2("Balance")
    .trend(
        df.features("age").as_series().to_vec<double>(),
        df.features("balance").as_series().to_vec<double>())
    .title("balance by age");
fig.save_svg("balance.svg");
```

See [PlussPl User Guide](plusspl_user_guide.md) and [Flash User Guide](flash_user_guide.md).

## Save results

```cpp
df.save_csv("outputs/clean.csv");
df.save_excel("outputs/clean.xlsx", "Clean");
df.save_json("outputs/clean.json");
df.save_parquet("outputs/clean.parquet");
```

Use `save_as` when the target type is dynamic:

```cpp
df.save_as("csv", "outputs/clean.csv");
```

## Atlas workflows

Use `Atlas` when you have several related frames:

```cpp
auto first = df.slice(0, 100);
auto second = df.slice(100, 200);

pluss::table::Atlas atlas({first, second});
atlas.show();
```

An `Atlas` is useful for scenario analysis, repeated backtests, multiple sheets,
or stacked simulation outputs.

## Production checklist

Before pushing a Pluss data workflow into a user-facing application:

- Use typed loader options for non-trivial I/O.
- Call `info()` and `describe()` during development to confirm inferred types.
- Prefer `query` for readable predicates and explicit masks for custom C++ logic.
- Keep case/index choices deliberate with `set_cases`, `reset_cases`, and loader
  `case_col` options.
- Use non-mutating transforms in library code unless mutation is intentional.
- Save intermediate artifacts with explicit filenames and formats.
