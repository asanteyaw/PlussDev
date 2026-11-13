<!-- pluss-hand-authored -->

# SlateTimelineAccessor::valid

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
Slate SlateTimelineAccessor::valid() const
```

## What It Does

Replaces the bound slate with a boolean `Series` flagging whether each row is
a real (non-`NaT`) timestamp — the logical inverse of [`isnat()`](isnat.md).

## Parameters

`None`

## Returns

`Slate`
    A new `Bool`-typed `Series` slate, same length as the bound slate:
    `true` where the row is a real timestamp, `false` where it's `NaT`.

## Examples

Example 1. Flag which rows are real timestamps.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-10T08:00:00Z", "", "2024-03-05T17:30:00Z"}));
auto out = events.tl().valid();
std::cout << out;
```

### Output

```text
0  true
1  false
2  true

ptype: Series
```

Row `1` — the missing timestamp — flags `false` here.

## Notes

- **`valid()` is one of the few `.tl()` methods whose own output is never
  masked to `"NULL"`** — it exists precisely to answer "is this row real," so
  the `NaT` row shows `false`, not `NULL`. Nearly every other `.tl()` method
  (the calendar-field extractors, `fmt()`, `epoch()`, and so on) instead shows
  `"NULL"` for a `NaT` row.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.

## See Also

- [`SlateTimelineAccessor::isnat`](isnat.md): the logical inverse (with the
  same never-`NULL` behavior).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="utc.html">
    <span class="label">Previous</span>
    <span class="title">utc</span>
  </a>
  <a class="next-link" href="week.html">
    <span class="label">Next</span>
    <span class="title">week</span>
  </a>
</div>
