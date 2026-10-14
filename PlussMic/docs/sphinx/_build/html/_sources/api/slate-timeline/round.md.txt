<!-- pluss-hand-authored -->

# SlateTimelineAccessor::round

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
SlateTimelineAccessor::round(std::string_view unit) const -> Slate
```

## What It Does

Rounds every row of the bound slate to the *nearest* `unit` boundary, in
local wall time — same length, still `Timeline`-kind.

## Parameters

**unit** (`std::string_view`)

One of `"S"`/`"s"`/`"sec"`/`"second"` (second), `"min"`/`"m"`/`"minute"`
(minute), `"H"`/`"h"`/`"hour"` (hour), or `"D"`/`"d"`/`"day"` (day). Cannot be
empty.

## Returns

`Slate`
    A new `Timeline`-kind slate, same length as the bound slate,
    rounded to the nearest `unit`.

## Examples

Example 1. Round every row to the nearest hour.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-06-01T09:47:12Z",
    "2024-06-01T14:05:00Z",
}));
auto out = events.tl().round("H");
std::cout << out;
```

### Output

```text
0  2024-06-01 10:00:00
1  2024-06-01 14:00:00

ptype: Timeline
```

`09:47:12` is closer to `10:00:00` than `09:00:00`, so it rounds up;
`14:05:00` is closer to `14:00:00` than `15:00:00`, so it rounds down.

Example 2. A row that sits *exactly* halfway between two hour boundaries —
confirming the tie-break direction.

```cpp
auto tie = Slate(pluss::make_timeline({"2024-06-01T09:30:00Z"}));
auto out2 = tie.tl().round("H");
std::cout << out2;
```

### Output

```text
0  2024-06-01 10:00:00

ptype: Timeline
```

`09:30:00` is exactly halfway between `09:00:00` and `10:00:00`. The tie
resolves **up** — this method never breaks a tie downward.

Example 3. An unsupported unit throws — the same message
[`floor()`](floor.md)/[`ceil()`](ceil.md) produce.

```cpp
try {
  auto bad = events.tl().round("week");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Timeline: unsupported unit (expected S/min/H/D)
```

## Notes

- Rounding happens in **local wall time** (per the slate's current
  timezone), then maps back.
- `unit` cannot be empty, and only accepts the fixed clock units listed above.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row stays `NaT` (displays as `"NULL"`) regardless of `unit`.

## See Also

- [`SlateTimelineAccessor::floor`](floor.md),
  [`SlateTimelineAccessor::ceil`](ceil.md): always down, or always up,
  instead of nearest.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter.html">
    <span class="label">Previous</span>
    <span class="title">quarter</span>
  </a>
  <a class="next-link" href="second.html">
    <span class="label">Next</span>
    <span class="title">second</span>
  </a>
</div>
