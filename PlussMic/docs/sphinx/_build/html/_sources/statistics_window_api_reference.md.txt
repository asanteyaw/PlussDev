# Window Statistics API Reference

Window analytics live on the time-series surface.

Use selected numeric value features first, then call `.ts()`:

```cpp
auto values = df.features({"close", "volume"});

auto rolling_mean = values.ts().rolling(
    20,
    pluss::table::WindowStatistic::Mean,
    pluss::table::WindowOptions{.min_count = 10});

auto expanding_var = values.ts().expanding(
    pluss::table::WindowStatistic::Var,
    pluss::table::StatisticOptions{.ddof = 1, .min_count = 2});

auto ewm_mean = values.ts().ewm(
    0.2,
    pluss::table::WindowStatistic::Mean);
```

For one-dimensional data, use the same surface:

```cpp
auto close = df.features("close");
auto rolling_sum = close.ts().rolling(5, pluss::table::WindowStatistic::Sum);
```

Grouped and time-axis windows are also on `.ts()`:

```cpp
auto grouped = df.features("score").ts().grouped(
    df.features("case_type"),
    5,
    pluss::table::WindowStatistic::Mean);

auto timed = df.features("score").ts().timed(
    df.features("timestamp"),
    7LL * 24 * 60 * 60 * 1000 * 1000 * 1000,
    pluss::table::TimeWindowStatistic::Variance,
    pluss::table::StatisticOptions{.ddof = 1, .min_count = 2},
    pluss::table::WindowClosure::Right);
```

Direct `DataFrame::moving`, `DataFrame::growing`, `DataFrame::decay`,
`DataFrame::grouped`, `DataFrame::timed`, and direct `Thread` variants are not
part of the public API. Select intentional time-series values and use `.ts()`.
