<!-- pluss-hand-authored -->

# SlateTimelineAccessor::epoch

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
SlateTimelineAccessor::epoch(TimeUnit unit = TimeUnit::Nanos) const -> Slate
```

## What It Does

Replaces the bound slate with its Unix epoch value in `unit`, as a numeric
`Series`.

## Parameters

**unit** (`TimeUnit`, default `TimeUnit::Nanos`)

One of the fixed-duration units: `Nanos`, `Micros`, `Millis`, `Seconds`, or
`Days`. Passing a span-only unit (`Weeks`, `Months`, `Years`,
`BusinessDays` — the calendar-relative units meant for [`span()`](span.md), not
a fixed epoch count) throws.

## Returns

`Slate`
    A new `Series`-kind (`Int64`) slate, same length as the bound slate,
    holding each row's epoch count in `unit`.

## Examples

Example 1. Extract the epoch second from every row.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "1970-01-01T00:00:00Z",
    "2024-01-01T00:00:10Z",
}));
auto out = events.tl().epoch(TimeUnit::Seconds);
std::cout << out;
```

### Output

```text
0  0
1  1704067210

ptype: Series
```

Example 2. Call `epoch()` with no argument to confirm the default unit is
`Nanos`, not `Seconds`.

```cpp
auto out2 = events.tl().epoch();
std::cout << out2;
```

### Output

```text
0  0
1  1704067210000000000

ptype: Series
```

Example 3. Pass a span-only unit to see the call throw.

```cpp
try {
  auto bad = events.tl().epoch(TimeUnit::Months);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::tl().epoch: unit is span-only; use Slate::tl().span
```

`Months` (like `Weeks`, `Years`, and `BusinessDays`) only makes sense as a
calendar-relative distance between two timestamps, so `epoch()` rejects it
outright — see [`span()`](span.md) for the method that does accept it.

## Notes

- The default unit is `Nanos`, not `Seconds`.
- Only fixed-duration units are accepted (`Nanos`/`Micros`/`Millis`/`Seconds`/
  `Days`); passing `Weeks`, `Months`, `Years`, or `BusinessDays` throws.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row (or one already null on the underlying slate) displays as
  `"NULL"`, not a sentinel epoch value.

## See Also

- [`SlateTimelineAccessor::span`](span.md): elapsed distance *between* two
  timelines, including calendar-relative units `epoch()` doesn't support.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="day.html">
    <span class="label">Previous</span>
    <span class="title">day</span>
  </a>
  <a class="next-link" href="floor.html">
    <span class="label">Next</span>
    <span class="title">floor</span>
  </a>
</div>
