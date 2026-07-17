# Thread::bounded

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::bounded(const WindowBoundsFn& bounds, WindowStatistic statistic = WindowStatistic::Mean, const WindowOptions& options = {}) -> Thread
```

Compute custom-window statistics on one thread by supplying explicit per-row bounds.

`Thread::bounded(...)` is the one-dimensional version of custom windowing. It is the right tool when fixed rolling widths are not expressive enough and each row should choose its own window.

## Parameters

**bounds**

Callback that generates the per-row window boundaries.

- It receives the thread size and active `WindowOptions`.
- It returns `starts` and `ends`, one pair per output row.
- Each row’s window is interpreted as `[start, end)`.

**statistic**

Window statistic to compute inside each row’s bounds.

Common values include `WindowStatistic::Mean`, `Sum`, `Minimum`, `Maximum`, `Var`, and `Std`.

**options**

Additional window policy.

Important fields include:

- `min_count`: minimum observed values required for a valid output.
- `ddof`: degrees of freedom for variance-style statistics.
- `centered`: centering hint for methods that honor it.
- `closure`: interval-closure policy for methods that honor it.

## Returns

`Thread`
    One output value per input row after applying the requested window statistic.

## See Also

- <a href="../dataframe/ts.html"><code>DataFrame::ts()</code></a>: open the DataFrame time-series surface, then call <code>.bounded(...)</code> across selected numeric features.
- <a href="#rolling"><code>Thread::rolling</code></a>: use fixed-width rolling windows instead of custom row bounds.

## Notes

The bounds callback returns half-open intervals `[start, end)` for each output row.

`WindowOptions::min_count` controls how many observed values must be present before one row’s result becomes valid.

## Examples

Example 1. Compute an expanding mean from the first row through the current row.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = scores.bounded(
    [](std::int64_t size, const WindowOptions&) {
      std::vector<std::int64_t> starts(static_cast<std::size_t>(size), 0);
      std::vector<std::int64_t> ends;
      ends.reserve(static_cast<std::size_t>(size));
      for (std::int64_t row = 0; row < size; ++row) {
        ends.push_back(row + 1);
      }
      return std::pair{std::move(starts), std::move(ends)};
    },
    WindowStatistic::Mean);
```

### Output

```text
0  10
1  15
2  20
3  25
4  30
dtype: Series
```

Example 2. Compute a trailing-two-row mean and require at least two observed values.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = scores.bounded(
    [](std::int64_t size, const WindowOptions&) {
      std::vector<std::int64_t> starts;
      std::vector<std::int64_t> ends;
      starts.reserve(static_cast<std::size_t>(size));
      ends.reserve(static_cast<std::size_t>(size));
      for (std::int64_t row = 0; row < size; ++row) {
        starts.push_back(std::max<std::int64_t>(0, row - 1));
        ends.push_back(row + 1);
      }
      return std::pair{std::move(starts), std::move(ends)};
    },
    WindowStatistic::Mean,
    WindowOptions{.min_count = 2});
```

### Output

```text
0  NA
1  15
2  25
3  35
4  45
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bottom.html">
    <span class="label">Previous</span>
    <span class="title">bottom</span>
  </a>
  <a class="next-link" href="btw.html">
    <span class="label">Next</span>
    <span class="title">btw</span>
  </a>
</div>
