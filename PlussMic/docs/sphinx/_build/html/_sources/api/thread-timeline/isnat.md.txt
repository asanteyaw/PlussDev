<!-- pluss-hand-authored -->

# ThreadTimelineAccessor::isnat

[Back to ThreadTimelineAccessor API Reference](../../thread_timeline_accessor_api_reference)

## Signature

```cpp
ThreadTimelineAccessor::isnat() const -> Thread
```

## What It Does

Replaces the bound thread with a boolean `Series` flagging whether each row is
`NaT` (missing timestamp).

## Parameters

`None`

## Returns

`Thread`
    A new `Bool`-typed `Series` thread, same length as the bound thread:
    `true` where the row is `NaT`, `false` otherwise.

## Examples

Example 1. Flag which rows are `NaT`.

```cpp
using namespace pluss::table;

auto events = Thread(pluss::make_timeline({
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
- `.tl()` requires the bound thread to already be `Timeline`/`TimeIndex`-kind —
  the accessor's constructor throws otherwise.

## See Also

- [`ThreadTimelineAccessor::valid`](valid.md): the logical inverse (with the
  same never-`NULL` behavior).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index.html">
    <span class="label">Previous</span>
    <span class="title">index</span>
  </a>
  <a class="next-link" href="iso.html">
    <span class="label">Next</span>
    <span class="title">iso</span>
  </a>
</div>
