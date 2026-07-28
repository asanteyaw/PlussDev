<!-- pluss-hand-authored -->

# DataFrameTimelineAccessor::index

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::index(bool require_sorted = true) const -> DataFrame
```

## What It Does

Replaces every selected `Timeline` feature with an equivalent `TimeIndex` —
same feature name, same row order, same displayed values. `TimeIndex` is the
monotonic time-axis type other parts of the library (like duration-based
`.ts().resample()`/`.ts().rolling(duration)`) expect. A feature already
`TimeIndex`-kind is passed through unchanged.

## Parameters

**require_sorted** (`bool`, default `true`)

When `true` (the default), every value must be non-decreasing and non-`NaT`,
or the call throws. When `false`, the conversion happens without checking
order or `NaT` — but the result may then misbehave in code elsewhere that
assumes a genuinely monotonic `TimeIndex`.

## Returns

`DataFrame`
    A new frame with each selected `Timeline` feature converted to
    `TimeIndex`.

## Examples

Example 1. Convert an already-sorted `Timeline` feature to `TimeIndex`.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-01T08:00:00Z",
          "2024-01-02T08:00:00Z",
          "2024-01-03T08:00:00Z",
      }))}});
std::cout << events;

auto out = events.tl().index();
std::cout << out;
```

### Output (before, Timeline)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-01 08:00:00
1    | 2024-01-02 08:00:00
2    | 2024-01-03 08:00:00
--------------------------
[3 cs x 1 feat]
```

### Output (after, TimeIndex)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-01 08:00:00
1    | 2024-01-02 08:00:00
2    | 2024-01-03 08:00:00
--------------------------
[3 cs x 1 feat]
```

The printed table looks identical — `TimeIndex` renders exactly like
`Timeline` — but the feature's underlying kind is now `TimeIndex`, which is
what a duration-based call like `.ts().resample("D", ...)` requires as its
time axis.

Example 2. Call `index()` on an out-of-order `Timeline` with the default
`require_sorted = true`, to see it throw.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-03T00:00:00Z",
          "2024-01-01T00:00:00Z",
          "2024-01-02T00:00:00Z",
      }))}});
std::cout << events;

try {
  auto out = events.tl().index();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output (before)

```text
=================
     | ts        
-----------------
0    | 2024-01-03
1    | 2024-01-01
2    | 2024-01-02
-----------------
[3 cs x 1 feat]
```

### Output (thrown message)

```text
TimeIndex must be sorted (non-decreasing)
```

Row `0` (`01-03`) is later than row `1` (`01-01`), so the sequence isn't
non-decreasing — with `require_sorted` left at its default, the call refuses
to produce a `TimeIndex` from it.

Example 3. Call `index(false)` on the same out-of-order data, to see it pass
through without checking.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-03T00:00:00Z",
          "2024-01-01T00:00:00Z",
          "2024-01-02T00:00:00Z",
      }))}});

auto out = events.tl().index(false);
std::cout << out;
```

### Output

```text
=================
     | ts        
-----------------
0    | 2024-01-03
1    | 2024-01-01
2    | 2024-01-02
-----------------
[3 cs x 1 feat]
```

With `require_sorted = false`, the same three out-of-order rows convert
successfully to `TimeIndex` — the ordering check is skipped entirely, and the
rows keep their original (unsorted) order. Nothing here stops the caller from
later feeding this into a duration-based method that quietly assumes a
monotonic axis.

Example 4. Call `index()` on a `Timeline` containing `NaT`, to see the
distinct "does not allow NaT" throw.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-01T00:00:00Z",
          "",
          "2024-01-03T00:00:00Z",
      }))}});
std::cout << events;

try {
  auto out = events.tl().index();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-01 00:00:00
1    | NULL               
2    | 2024-01-03 00:00:00
--------------------------
[3 cs x 1 feat]
```

### Output (thrown message)

```text
TimeIndex does not allow NaT
```

Row `1` is otherwise "in order" (it sits between two rows that are already
increasing), but its value is `NaT` — `require_sorted = true` rejects that on
its own, with a different message than the plain-unsorted case above.

## Notes

- With the default `require_sorted = true`, calling this on an *unsorted*
  `Timeline` (or one containing `NaT`) throws immediately — sort with
  [`DataFrame::arrange`](../dataframe/arrange.md) (or an equivalent)
  first if needed.
- `require_sorted = false` skips that check entirely — use it only when you
  already know the data's order doesn't matter for what comes next, since
  downstream duration-based operations generally *do* assume a monotonic
  axis.
- A feature that's already `TimeIndex`-kind is returned as-is, regardless of
  `require_sorted`.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.

## See Also

- [`DataFrame::ts`](../dataframe/ts.md): the accessor whose duration-based
  methods (`resample`, `rolling(duration)`, and similar) expect a
  `TimeIndex` axis like the one this method produces.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
