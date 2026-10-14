<!-- pluss-hand-authored -->

# SlateTimelineAccessor::isnat

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
SlateTimelineAccessor::isnat() const -> Slate
```

## What It Does

Replaces the bound slate with a boolean `Series` flagging whether each row is
`NaT` (missing timestamp).

## Parameters

`None`

## Returns

`Slate`
    A new `Bool`-typed `Series` slate, same length as the bound slate:
    `true` where the row is `NaT`, `false` otherwise.

## Examples

Example 1. Flag which rows are `NaT`.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-10T08:00:00Z", "", "2024-03-05T17:30:00Z"}));
auto out = events.tl().isnat();
std::cout << out;
```

### Output

```text
0  false
1  true
2  false

ptype: Series
```

Row `1` — the missing timestamp — flags `true` here.

## Notes

- **`isnat()` is one of the few `.tl()` methods whose own output is never
  masked to `"NULL"`** — the whole point of the method is to answer "is this
  row missing," so the row that *is* `NaT` shows `true`, not `NULL`. Nearly
  every other `.tl()` method (the calendar-field extractors, `fmt()`,
  `epoch()`, and so on) instead shows `"NULL"` for a `NaT` row.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.

## See Also

- [`SlateTimelineAccessor::valid`](valid.md): the logical inverse (with the
  same never-`NULL` behavior).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="iso.html">
    <span class="label">Next</span>
    <span class="title">iso</span>
  </a>
</div>
