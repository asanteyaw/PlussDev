<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ad6405435146 -->

# DataFrameTsAccessor::sort

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::sort(bool ascending = true, bool nulls_last = true) const -> DataFrame
```

## What It Does

Reorders every bound feature's rows into chronological order along the accessor's
time axis. That axis is either an explicit `TsOptions.axis` (if supplied when
`.ts()` was called), or — when no explicit axis is set — the frame's own case
labels, which must already be `Timeline`/`TimeIndex` kind (this method throws
otherwise). The result carries the same time axis, now sorted, as its case labels.

## Parameters

**ascending** (`bool`, default `true`)

Sort chronologically forward (`true`) or backward (`false`).

**nulls_last** (`bool`, default `true`)

Whether undefined axis positions sort after (`true`) or before (`false`) every
defined position.

## Returns

`DataFrame`
    A new frame containing the accessor's bound features, reordered into
    chronological order along the time axis, with that axis as its case labels.

## Examples

Example 1. Sort a frame whose case labels are an out-of-order `Timeline`.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"value", Thread(pluss::make_series({30.0, 10.0, 20.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-03T00:00:00Z",
        "2024-01-01T00:00:00Z",
        "2024-01-02T00:00:00Z",
    })));
std::cout << readings;
```

### Output (before)

```text
==================
           | value
------------------
2024-01-03 | 30   
2024-01-01 | 10   
2024-01-02 | 20   
------------------
[3 cs x 1 feat]
```

```cpp
auto out = readings.ts().sort();
std::cout << out;
```

### Output (after)

```text
==================
           | value
------------------
2024-01-01 | 10   
2024-01-02 | 20   
2024-01-03 | 30   
------------------
[3 cs x 1 feat]
```

`readings`'s case labels are already a `Timeline`, so `readings.ts()` (with no
explicit `TsOptions.axis`) can use them directly as the time axis. `sort()`
reorders the rows chronologically — `2024-01-01`, then `2024-01-02`, then
`2024-01-03` — carrying each row's `value` along with it.

Example 2. Pass `ascending = false` to sort the same frame in reverse
chronological order.

```cpp
auto descending = readings.ts().sort(false);
std::cout << descending;
```

### Output

```text
==================
           | value
------------------
2024-01-03 | 30   
2024-01-02 | 20   
2024-01-01 | 10   
------------------
[3 cs x 1 feat]
```

The same three rows, now ordered latest-to-earliest — `2024-01-03`, then
`2024-01-02`, then `2024-01-01` — the mirror image of Example 1's ascending
result.

Example 3. Pass `nulls_last = false` to change where an undefined (`NaT`)
axis position sorts.

```cpp
auto with_nat = DataFrame(
    {{"value", Thread(pluss::make_series({99.0, 20.0, 10.0}))}},
    Thread(pluss::make_timeline({
        "",
        "2024-01-02T00:00:00Z",
        "2024-01-01T00:00:00Z",
    })));
std::cout << with_nat;

auto nat_last = with_nat.ts().sort();
std::cout << nat_last;

auto nat_first = with_nat.ts().sort(true, false);
std::cout << nat_first;
```

### Output (with_nat)

```text
==================
           | value
------------------
NULL       | 99   
2024-01-02 | 20   
2024-01-01 | 10   
------------------
[3 cs x 1 feat]
```

### Output (nulls_last = true, the default)

```text
==================
           | value
------------------
2024-01-01 | 10   
2024-01-02 | 20   
NULL       | 99   
------------------
[3 cs x 1 feat]
```

### Output (nulls_last = false)

```text
==================
           | value
------------------
NULL       | 99   
2024-01-01 | 10   
2024-01-02 | 20   
------------------
[3 cs x 1 feat]
```

The empty-string constructor produces a `NaT` axis position, displayed as
`NULL`. With the default `nulls_last = true`, it sorts after every real
timestamp; with `nulls_last = false`, it sorts before them instead — the same
convention [`DataFrameLabelAccessor::monotonic`](../dataframe-label/monotonic.md)
and [`sort`](../dataframe-label/sort.md) use.

Example 4. Call `sort()` on a frame whose case labels are a plain numeric
index, with no explicit `TsOptions.axis`, to see it throw.

```cpp
using namespace pluss::table;

auto untimed = DataFrame(
    {{"value", Thread(pluss::make_series({30.0, 10.0, 20.0}))}});

try {
  auto out = untimed.ts().sort();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrameTsAccessor::sort: cases must be Timeline/TimeIndex. Use df.label().set("date") first
```

`untimed`'s case labels are the default integer index — no `Timeline`/
`TimeIndex`, and no explicit `TsOptions.axis` to fall back on — so there's no
time axis to sort by.

## Notes

- With no explicit `TsOptions.axis`, this method requires the frame's own case
  labels to already be `Timeline`/`TimeIndex` — it throws if they're a plain
  numeric or `Note` index instead.
- This is distinct from [`DataFrame::label().sort()`](../dataframe/label.md),
  which sorts by whatever kind the case labels happen to be; `ts().sort()` is
  specifically the time-axis-aware entry point within the `ts()` accessor.

## See Also

- [`DataFrameTsAccessor::rolling`](rolling.md) (calendar-duration overload),
  [`DataFrameTsAccessor::resample`](resample.md): other methods that rely on the
  same time-axis requirement this method sorts by.
- [`DataFrame::label`](../dataframe/label.md): the general case-label accessor,
  including its own `sort()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling_block.html">
    <span class="label">Previous</span>
    <span class="title">rolling_block</span>
  </a>
  <a class="next-link" href="sub.html">
    <span class="label">Next</span>
    <span class="title">sub</span>
  </a>
</div>
