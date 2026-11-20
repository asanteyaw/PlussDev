<!-- pluss-hand-authored -->

# ShelfTimelineAccessor::floor

[Back to ShelfTimelineAccessor API Reference](../../shelf_timeline_accessor_api_reference)

## Signature

```cpp
Shelf ShelfTimelineAccessor::floor(std::string_view unit) const
```

## What It Does

Rounds every row of the bound shelf *down* to the nearest `unit` boundary, in
local wall time — same length, still `Timeline`-kind.

## Parameters

**unit** (`std::string_view`)

One of `"S"`/`"s"`/`"sec"`/`"second"` (second), `"min"`/`"m"`/`"minute"`
(minute), `"H"`/`"h"`/`"hour"` (hour), or `"D"`/`"d"`/`"day"` (day). Cannot be
empty. Coarser units (weeks, months) aren't supported.

## Returns

`Shelf`
    A new `Timeline`-kind shelf, same length as the bound shelf,
    floored to `unit`.

## Examples

Example 1. Floor every row to the nearest hour.

```cpp
using namespace pluss::table;

auto events = Shelf(pluss::make_timeline({
    "2024-06-01T09:47:12Z",
    "2024-06-01T14:05:00Z",
}));
auto out = events.tl().floor("H");
std::cout << out;
```

### Output

```text
0  2024-06-01 09:00:00
1  2024-06-01 14:00:00

ptype: Timeline
```

Example 2. Floor to the nearest day *after* relabeling the display timezone —
`"D"` aligns to **local** midnight, not UTC midnight.

```cpp
auto ts = Shelf(pluss::make_timeline({"2024-01-15T03:00:00Z"}));
auto local = ts.tl().tz("America/New_York");
std::cout << local;

auto out2 = local.tl().floor("D");
std::cout << out2;
```

### Output (before)

```text
0  2024-01-14 22:00:00
ptype: Timeline
```

### Output (after)

```text
0  2024-01-14
ptype: Timeline
```

`2024-01-15T03:00:00Z` is `2024-01-14 22:00:00` in `America/New_York` (UTC-5 in
January); floored in that local wall time, it lands on `2024-01-14`'s midnight
— a full UTC day *earlier* than the UTC calendar date would suggest.

Example 3. An unsupported unit throws.

```cpp
try {
  auto bad = events.tl().floor("week");
} catch (const pluss::Error& e) {
  std::string msg(e.message());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Timeline: unsupported unit (expected S/min/H/D)
```

## Notes

- Flooring happens in **local wall time** (per the shelf's current timezone),
  then maps back — so `floor("D")` aligns to local midnight, not necessarily
  UTC midnight, if the timezone isn't `"UTC"`.
- `unit` cannot be empty, and only accepts the fixed clock units listed above —
  see [`period()`](period.md) for month/year truncation instead.
- `.tl()` requires the bound shelf to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row stays `NaT` (displays as `"NULL"`) regardless of `unit`.

## See Also

- [`ShelfTimelineAccessor::ceil`](ceil.md),
  [`ShelfTimelineAccessor::round`](round.md): round up, or to the nearest
  boundary, instead of always down.
- [`ShelfTimelineAccessor::date`](date.md): shorthand for `floor("D")`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch.html">
    <span class="label">Previous</span>
    <span class="title">epoch</span>
  </a>
  <a class="next-link" href="fmt.html">
    <span class="label">Next</span>
    <span class="title">fmt</span>
  </a>
</div>
