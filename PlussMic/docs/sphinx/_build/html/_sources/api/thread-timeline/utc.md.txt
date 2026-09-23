<!-- pluss-hand-authored -->

# ThreadTimelineAccessor::utc

[Back to ThreadTimelineAccessor API Reference](../../thread_timeline_accessor_api_reference)

## Signature

```cpp
ThreadTimelineAccessor::utc() const -> Thread
```

## What It Does

Replaces the bound thread's display timezone with `"UTC"` — same length,
still `Timeline`/`TimeIndex`-kind. Exactly equivalent to [`tz("UTC")`](tz.md);
like `tz()`, the underlying instant never changes, only the timezone used to
display it and compute local calendar fields.

## Parameters

`None`

## Returns

`Thread`
    A new `Timeline`/`TimeIndex`-kind thread, same length as the bound
    thread, with its display timezone switched to `"UTC"`.

## Examples

Example 1. Convert an `"America/New_York"`-labeled thread back to UTC
display.

```cpp
using namespace pluss::table;

auto events = Thread(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-07-15T09:15:30Z",
}));
auto ny = events.tl().tz("America/New_York");
std::cout << ny;

auto out = ny.tl().utc();
std::cout << out;
```

### Output (before, displayed in America/New_York)

```text
0  2024-01-31 04:15:30
1  2024-07-15 05:15:30

ptype: Timeline
```

### Output (after utc())

```text
0  2024-01-31 09:15:30
1  2024-07-15 09:15:30

ptype: Timeline
```

`utc()` undoes exactly what the earlier `tz("America/New_York")` call did —
same instants throughout, only the display timezone changes back.

## Notes

- Purely a display-timezone relabel — the underlying instant is unchanged, so
  chaining `.tz(name).utc()` always round-trips back to the original
  displayed values.
- `.tl()` requires the bound thread to already be `Timeline`/`TimeIndex`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row stays `NaT`.

## See Also

- [`ThreadTimelineAccessor::tz`](tz.md): the general form this method is a
  fixed `"UTC"` shorthand for.
- [`ThreadTimelineAccessor::localize`](localize.md): the operation that
  actually changes the underlying instant, unlike this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz.html">
    <span class="label">Previous</span>
    <span class="title">tz</span>
  </a>
  <a class="next-link" href="valid.html">
    <span class="label">Next</span>
    <span class="title">valid</span>
  </a>
</div>
