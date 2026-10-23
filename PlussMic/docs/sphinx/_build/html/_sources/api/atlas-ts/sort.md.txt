<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: aa227b7d86b8 -->

# AtlasTsAccessor::sort

[Back to AtlasTsAccessor API Reference](../../atlas_ts_accessor_api_reference)

## Signature

```cpp
AtlasTsAccessor::sort(bool ascending = true, bool nulls_last = true) const -> Atlas
```

## What It Does

Reorders every bound feature's rows into chronological order along each frame's own
time axis — the same transform
[`DataFrameTsAccessor::sort`](../dataframe-ts/sort.md) applies to a single frame,
broadcast across the whole `Atlas` via [`Atlas::map_frames()`](../atlas/map_frames.md).
That axis is each frame's own case labels, which must already be
`Timeline`/`TimeIndex` kind — this method throws otherwise.

## Parameters

**ascending** (`bool`, default `true`)
    Sort chronologically forward (`true`) or backward (`false`), within each frame.

**nulls_last** (`bool`, default `true`)
    Whether an undefined axis position sorts after (`true`) or before (`false`)
    every defined position, within each frame.

## Returns

`Atlas`
    A new atlas, same depth and depth labels as the source, with each frame's bound
    features reordered into chronological order, using that frame's time axis as its
    case labels.

## Examples

Example 1. Sort two frames whose case labels are each an out-of-order `Timeline`, on
different months.

```cpp
using namespace pluss::table;

auto q1 = DataFrame(
    {{"value", Slate(pluss::make_series({30.0, 10.0, 20.0}))}},
    Slate(pluss::make_timeline({
        "2024-01-03T00:00:00Z", "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"})));
auto q2 = DataFrame(
    {{"value", Slate(pluss::make_series({99.0, 55.0, 77.0}))}},
    Slate(pluss::make_timeline({
        "2024-02-03T00:00:00Z", "2024-02-01T00:00:00Z", "2024-02-02T00:00:00Z"})));
auto atlas = Atlas({q1, q2});

auto out = atlas.ts().sort();
std::cout << out.frame(0) << "\n" << out.frame(1);
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 10   
2024-01-02 | 20   
2024-01-03 | 30   
------------------
[3 cs x 1 feat]
==================
           | value
------------------
2024-02-01 | 55   
2024-02-02 | 77   
2024-02-03 | 99   
------------------
[3 cs x 1 feat]
```

Each frame is sorted along its own time axis — `frame(1)`'s February dates are
untouched by `frame(0)`'s January reordering.

Example 2. Call `sort()` on an atlas whose frames have plain numeric case labels, no
explicit `Timeline`, to see it throw.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({{"load", Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))}});
auto q2 = DataFrame({{"load", Slate(pluss::make_series({12.0, 18.0, 14.0, 28.0}))}});
auto atlas = Atlas({q1, q2});

try {
    auto out = atlas.ts().sort();
} catch (const std::exception& e) {
    std::string msg(e.what());
    std::cout << "threw: " << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
threw: DataFrameTsAccessor::sort: cases must be Timeline/TimeIndex. Use df.labels().set("date") first
```

`atlas.ts()` binds per-frame accessors that delegate straight to
[`DataFrame::ts()`](../dataframe/ts.md), so the thrown message names
`DataFrameTsAccessor`, not `AtlasTsAccessor` — the check happens once per frame, the
moment `map_frames()` reaches a frame with no time axis.

## Notes

- With no explicit `TsOptions.axis`, this method requires each frame's own case
  labels to already be `Timeline`/`TimeIndex` — it throws if any frame's are a plain
  numeric or `Note` index instead.
- Each frame is sorted independently — no row ever moves from one frame to another.

## See Also

- [`DataFrameTsAccessor::sort`](../dataframe-ts/sort.md): the single-frame operation
  this method broadcasts across the stack.
- [`AtlasTsAccessor::rolling`](rolling.md) (calendar-duration overload),
  [`AtlasTsAccessor::resample`](resample.md): other methods that rely on the same
  time-axis requirement this method sorts by.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling.html">
    <span class="label">Previous</span>
    <span class="title">rolling</span>
  </a>
</div>
