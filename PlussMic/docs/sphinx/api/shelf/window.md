<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f3192dc46e3e -->

# Shelf::window

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::window(std::int64_t window, ShelfWindowOp op, std::int64_t min_periods = 1) const
```

## What It Does

Computes a **trailing** rolling aggregate over a numeric `Series`-kind shelf: row `i` of
the result summarizes rows `[i - window + 1, i]` of the input (clipped at the start of the
shelf for the first `window - 1` rows), using whichever aggregate `op` selects.

### `ShelfWindowOp` values

| Value | Numeric value | Description |
|---|---|---|
| `RollingSum` | `0` | — |
| `RollingMean` | `1` | — |
| `RollingMin` | `2` | — |
| `RollingMax` | `3` | — |

`min_periods` sets how many *valid* values must actually be present within a row's window
for that row to produce a result at all — rows whose window has fewer valid entries than
`min_periods` come back `NULL`. With the default `min_periods = 1`, every row from the very
first one produces a value, computed over however many rows are actually available near the
start of the shelf (a genuine, shrinking window, not one padded with zeros).

```{important} min_periods, not window size, controls when NULL appears near the start
Near the start of the shelf, fewer than `window` rows are available. With the default
`min_periods = 1`, those early rows still produce a result over whatever's available (see
Example 1: row 0's window is just itself, row 1's is rows 0–1). Raise `min_periods` — up to
and including `window` itself — to require a *full* window before producing a value; rows
that don't yet have enough valid history come back `NULL` instead. See Example 3.
```

## Parameters

**window** (`std::int64_t`)

The trailing window size, in rows. Must be `> 0`.

**op** (`ShelfWindowOp`)

Which aggregate to compute over each window — `RollingSum`, `RollingMean`,
`RollingMin`, or `RollingMax`.

**min_periods** (`std::int64_t`, default `1`)

The minimum number of valid values required within a row's window for that row to
produce a result; rows with fewer are `NULL`. Must be `> 0` and `<= window`.

## Returns

`Shelf`
    A `Float64` `Series`-kind shelf, the same length as `*this`, holding one rolling
    aggregate per row.

## Examples

Example 1. `RollingMean` with the default `min_periods = 1` — the first two rows still
produce a value, over a shrinking (not full-size) window.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = values.window(3, ShelfWindowOp::RollingMean);
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
auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = values.window(3, ShelfWindowOp::RollingSum);
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
auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = values.window(3, ShelfWindowOp::RollingMean, 3);
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
auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

std::cout << values.window(2, ShelfWindowOp::RollingMin) << "\n";
std::cout << values.window(2, ShelfWindowOp::RollingMax);
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

Example 5. `window()` requires a numeric `Series`-kind shelf — calling it on any other
payload kind throws.

```cpp
auto notes = Shelf(pluss::make_note({"a", "b", "c"}));
try {
  auto out = notes.window(2, ShelfWindowOp::RollingMean);
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::window: only Series payload supports rolling ops
```

## Notes

- The window is trailing and inclusive of the current row: row `i` summarizes
  `[i - window + 1, i]`, clipped at index `0`.
- `min_periods` (not the window size) controls whether the shrinking window near the start
  of the shelf produces a value or `NULL` — see the important callout above.
- Requires a numeric `Series`-kind shelf; throws on `Note`/`Timeline`/`TimeIndex` payloads,
  and on `window <= 0` or `min_periods` outside `(0, window]`.
- The result is always `Float64`, regardless of the input's numeric dtype.

## See Also

- `Shelf::timed`: the time-duration-based rolling window, for `Timeline`-kind shelves with
  irregular spacing, instead of a fixed row count.
- `Shelf::grouped`: the same rolling-window computation, run independently within each
  group of a parallel grouping shelf.
- [`Shelf::scan`](scan.md): a cumulative (unbounded window) statistic, rather than a
  fixed-size rolling one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weigh.html">
    <span class="label">Previous</span>
    <span class="title">weigh</span>
  </a>
</div>
