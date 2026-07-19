# Part XIII: Recipes

This part gives task-oriented recipes. Copy the pattern, then adapt feature
names and options.

## Load, clean, summarize, save

```cpp
using pluss::table::DataFrame;

auto raw = DataFrame::load_csv("input.csv", {
    {"ncases", 10000},
});

auto clean = raw
    .fillnull(0.0)
    .dropnull({"id"}, true)
    .sort_values({"id"});

clean.describe();
clean.save_csv("outputs/clean.csv");
```

## Load selected features

```cpp
pluss::loader::ReadCsvOptions options;
options.usefeats = {"id", "date", "price", "volume"};
options.parse_dates = {"date"};
options.case_col = "id";

auto df = DataFrame::load_csv("trades.csv", options);
```

## Feature engineering pipeline

```cpp
auto features = df
    .fillnull(0.0, {"price", "volume"})
    .assign({{
        "notional",
        [](const DataFrame& frame) {
          return frame.features("price") * frame.features("volume");
        },
    }})
    .assign({{
        "log_volume",
        [](const DataFrame& frame) {
          return frame.features("volume").log();
        },
    }});
```

## Grouped report

```cpp
df.group_by(
    {"sector"},
    {
        {"n", "price", DataFrame::GroupOp::Count},
        {"avg_price", "price", DataFrame::GroupOp::Mean},
        {"max_volume", "volume", DataFrame::GroupOp::Max},
    },
    true,
    true);
```

## Top-N report

```cpp
auto top = df
    .sort_values({"revenue"}, false)
    .head(25, false);

top.save_csv("outputs/top_revenue.csv");
```

## Missing-value audit

```cpp
std::cout << "null rows: " << df.isnull().any(pluss::table::Axis::Case).sum() << '\n';
df.isnull().head(20);
df.tally({"category"}, 20, false, true);
```

## Text cleanup

```cpp
auto cleaned = df.transform({
    {"name", [](const auto& t) { return t.lower().replace(" inc.", ""); }},
    {"city", [](const auto& t) { return t.lower(); }},
});
```

## Time-series features

```cpp
auto sorted = df.sort_values({"date"});
auto engineered = sorted.assign({
    {"return_1", [](const DataFrame& frame) {
      return frame.features("close").ts().returns(1);
    }},
    {"close_20_mean", [](const DataFrame& frame) {
      return frame.features("close").ts().rolling(20, pluss::table::WindowStatistic::Mean, {.min_count = 10});
    }},
});
```

## Daily resample

```cpp
auto daily = df.features({"price", "volume"})
    .ts({.axis = df.features("timestamp")})
    .resample("D", pluss::subsp::stats::ts::ResampleOp::Mean);
```

## Excel workbook ingestion

```cpp
auto workbook = DataFrame::load_excel("report.xlsx", nullptr, true);

for (const auto& name : workbook.sheet_names().values()) {
  auto sheet = workbook.sheets(name);
  sheet.info();
}
```

## Multi-scenario Atlas

```cpp
auto base = DataFrame::load_csv("base.csv");
auto stress = DataFrame::load_csv("stress.csv");
auto optimistic = DataFrame::load_csv("optimistic.csv");

pluss::table::Thread labels =
    pluss::make_note({"base", "stress", "optimistic"});
pluss::table::Atlas scenarios({base, stress, optimistic}, labels);

auto cleaned = scenarios.map_frames([](const DataFrame& frame) {
  return frame.fillnull(0.0);
});
```

## Round-trip through Kindle and Bundle

```cpp
auto catalog = df.kindelize();
auto registry = pluss::bundle::Registry::from_kindle(catalog);

registry.type_profiles();

auto back = registry.to_kindle().tabelize();
```

## Save a report bundle

```cpp
auto clean = df.fillnull(0.0);
auto summary = clean.describe();
auto grouped = clean.group_by(
    {"group"},
    {{"mean_value", "value", DataFrame::GroupOp::Mean}},
    true,
    false);

clean.save_csv("outputs/clean.csv");
summary.save_csv("outputs/summary.csv");
grouped.save_csv("outputs/grouped.csv");
```
