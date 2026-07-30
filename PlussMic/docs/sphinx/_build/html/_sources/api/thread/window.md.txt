<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 107375e3e577 -->

# Thread::window

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::window(std::int64_t window, ThreadWindowOp op, std::int64_t min_periods = 1) const -> Thread
```

## What It Does

Computes a **trailing** rolling aggregate over a numeric `Series`-kind thread: row `i` of
the result summarizes rows `[i - window + 1, i]` of the input (clipped at the start of the
thread for the first `window - 1` rows), using whichever aggregate `op` selects.

```cpp
enum class ThreadWindowOp : std::uint8_t {
  RollingSum = 0,
  RollingMean = 1,
  RollingMin = 2,
  RollingMax = 3,
};
```

`min_periods` sets how many *valid* values must actually be present within a row's window
for that row to produce a result at all — rows whose window has fewer valid entries than
`min_periods` come back `NULL`. With the default `min_periods = 1`, every row from the very
first one produces a value, computed over however many rows are actually available near the
start of the thread (a genuine, shrinking window, not one padded with zeros).

```{important} min_periods, not window size, controls when NULL appears near the start
Near the start of the thread, fewer than `window` rows are available. With the default
`min_periods = 1`, those early rows still produce a result over whatever's available (see
Example 1: row 0's window is just itself, row 1's is rows 0–1). Raise `min_periods` — up to
and including `window` itself — to require a *full* window before producing a value; rows
that don't yet have enough valid history come back `NULL` instead. See Example 3.
```

## Parameters

**window** (`std::int64_t`)
    The trailing window size, in rows. Must be `> 0`.

**op** (`ThreadWindowOp`)
    Which aggregate to compute over each window — `RollingSum`, `RollingMean`,
    `RollingMin`, or `RollingMax`.

**min_periods** (`std::int64_t`, default `1`)
    The minimum number of valid values required within a row's window for that row to
    produce a result; rows with fewer are `NULL`. Must be `> 0` and `<= window`.

## Returns

`Thread`
    A `Float64` `Series`-kind thread, the same length as `*this`, holding one rolling
    aggregate per row.

## Examples

Example 1. `RollingMean` with the default `min_periods = 1` — the first two rows still
produce a value, over a shrinking (not full-size) window.

```cpp
using namespace pluss::table;

auto values = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = values.window(3, ThreadWindowOp::RollingMean);
std::cout << out;
```

### Output

```text
0  10
1  15
2  20
3  30
4  40
ptype: Series
```

Row `0`'s window is just `{10}` (mean `10`); row `1`'s is `{10, 20}` (mean `15`); row `2`
onward has a full 3-row window (e.g. row `2`: `{10, 20, 30}`, mean `20`).

Example 2. `RollingSum` over the same data and window.

```cpp
auto values = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = values.window(3, ThreadWindowOp::RollingSum);
std::cout << out;
```

### Output

```text
0  10
1  30
2  60
3  90
4  120
ptype: Series
```

Example 3. Raising `min_periods` to the full `window` size requires a complete window
before producing a value — the first `window - 1` rows come back `NULL` instead of a
partial-window result.

```cpp
auto values = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = values.window(3, ThreadWindowOp::RollingMean, 3);
std::cout << out;
```

### Output

```text
0  NULL
1  NULL
2  20
3  30
4  40
ptype: Series
```

Example 4. `RollingMin` and `RollingMax` over a 2-row trailing window.

```cpp
auto values = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

std::cout << values.window(2, ThreadWindowOp::RollingMin) << "\n";
std::cout << values.window(2, ThreadWindowOp::RollingMax);
```

### Output

```text
0  10
1  10
2  20
3  30
4  40
ptype: Series

0  10
1  20
2  30
3  40
4  50
ptype: Series
```

Example 5. `window()` requires a numeric `Series`-kind thread — calling it on any other
payload kind throws.

```cpp
auto notes = Thread(pluss::make_note({"a", "b", "c"}));
try {
  auto out = notes.window(2, ThreadWindowOp::RollingMean);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::window: only Series payload supports rolling ops
```

## Notes

- The window is trailing and inclusive of the current row: row `i` summarizes
  `[i - window + 1, i]`, clipped at index `0`.
- `min_periods` (not the window size) controls whether the shrinking window near the start
  of the thread produces a value or `NULL` — see the important callout above.
- Requires a numeric `Series`-kind thread; throws on `Note`/`Timeline`/`TimeIndex` payloads,
  and on `window <= 0` or `min_periods` outside `(0, window]`.
- The result is always `Float64`, regardless of the input's numeric dtype.

## See Also

- `Thread::timed`: the time-duration-based rolling window, for `Timeline`-kind threads with
  irregular spacing, instead of a fixed row count.
- `Thread::grouped`: the same rolling-window computation, run independently within each
  group of a parallel grouping thread.
- [`Thread::scan`](scan.md): a cumulative (unbounded window) statistic, rather than a
  fixed-size rolling one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weigh.html">
    <span class="label">Previous</span>
    <span class="title">weigh</span>
  </a>
</div>
