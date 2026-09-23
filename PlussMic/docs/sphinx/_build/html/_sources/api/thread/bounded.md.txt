<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8ee29534d666 -->

# Thread::bounded

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::bounded(const WindowBoundsFn& bounds, WindowStatistic statistic = WindowStatistic::Mean, const WindowOptions& options = {}) const -> Thread
```

## What It Does

Computes a window statistic over the thread using **custom, per-row window
boundaries** you supply — the general-purpose escape hatch behind
[`weigh()`](weigh.md) and the fixed-width rolling window methods on
[`Thread::ts()`](ts.md)'s `TimeSeriesAccessor` (`rolling()`, `expanding()`,
`timed()`, `grouped()` are all thin wrappers that compute their `starts`/`ends`
internally and delegate to the same machinery this method exposes directly).
Reach for `bounded()` when a fixed rolling width isn't expressive enough and
each output row needs its own, independently-computed window.

`bounds` is called once, given the thread's `size()` and the active
`WindowOptions`, and must return a `{starts, ends}` pair of equal-length
vectors — one `[start, end)` half-open interval per output row. Restricted to
numeric `Series` payloads; every other kind or dtype throws.

## Parameters

**bounds** (`const WindowBoundsFn&`)
    A callback `(size, options) -> {starts, ends}` producing one half-open
    `[start, end)` window per output row. Must not be empty (a default
    `std::function`), or it throws.

**statistic** (`WindowStatistic`, default `Mean`)
    Which statistic to compute inside each row's window: `Sum`, `Mean`,
    `Minimum`, `Maximum`, `Var`, or `Std`.

**options** (`WindowOptions`, default `{}`)
    `min_count` (default `1`): the smallest number of observed values inside a
    window still considered valid. `ddof` (default `1`): the divisor
    adjustment for `Var`/`Std`. `centered`/`closure` are accepted but have no
    effect here — your `bounds` callback already fully determines each
    window's shape.

## Returns

`Thread`
    A new thread the same length as the source, holding the requested
    statistic evaluated over each row's custom window.

## Examples

Example 1. An expanding mean — every row's window runs from the very start
through that row.

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
std::cout << out;
```

### Output

```text
0  10
1  15
2  20
3  25
4  30

ptype: Series
```

Each row's mean is over every value seen so far — `15` is `mean(10, 20)`, `20`
is `mean(10, 20, 30)`, and so on.

Example 2. A trailing two-row window, requiring at least two observed values
per window.

```cpp
auto out2 = scores.bounded(
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
std::cout << out2;
```

### Output

```text
0  NULL
1  15
2  25
3  35
4  45

ptype: Series
```

Row `0`'s window is just `[0, 1)` — only one observed value, below
`min_count = 2` — so it's `NULL`. Every later row has a genuine two-value
window: `15 = mean(10, 20)`, `25 = mean(20, 30)`, and so on.

## Notes

- `bounds` must return `starts`/`ends` vectors the same length as the source
  thread; each row's window is `[start, end)`, half-open.
- `centered` and `closure` in `options` have no effect here, since your
  `bounds` callback already fully controls each window's shape — they only
  matter for the fixed-width wrappers built on top of this method.
- An empty `bounds` callback throws `"Thread::bounded: bounds callback is
  empty"`.
- Restricted to numeric `Series` payloads.

## See Also

- [`Thread::weigh`](weigh.md): a fixed trailing window with an explicit
  per-position weight, rather than arbitrary custom bounds.
- [`Thread::ts`](ts.md): the time-series accessor whose fixed-width rolling
  methods (`rolling()`, `expanding()`, `timed()`, `grouped()`) are built on
  top of this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bottom.html">
    <span class="label">Previous</span>
    <span class="title">bottom</span>
  </a>
  <a class="next-link" href="bucketize.html">
    <span class="label">Next</span>
    <span class="title">bucketize</span>
  </a>
</div>
