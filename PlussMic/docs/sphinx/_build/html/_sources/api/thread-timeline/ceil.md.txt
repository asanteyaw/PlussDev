<!-- pluss-hand-authored -->

# ThreadTimelineAccessor::ceil

[Back to ThreadTimelineAccessor API Reference](../../thread_timeline_accessor_api_reference)

## Signature

```cpp
ThreadTimelineAccessor::ceil(std::string_view unit) const -> Thread
```

## What It Does

Rounds every row of the bound thread *up* to the nearest `unit` boundary, in
local wall time — same length, still `Timeline`-kind.

## Parameters

**unit** (`std::string_view`)

One of `"S"`/`"s"`/`"sec"`/`"second"` (second), `"min"`/`"m"`/`"minute"`
(minute), `"H"`/`"h"`/`"hour"` (hour), or `"D"`/`"d"`/`"day"` (day). Cannot be
empty.

## Returns

`Thread`
    A new `Timeline`-kind thread, same length as the bound thread,
    ceiled to `unit`.

## Examples

Example 1. Ceil every row to the nearest hour.

```cpp
using namespace pluss::table;

auto events = Thread(pluss::make_timeline({
    "2024-06-01T09:47:12Z",
    "2024-06-01T14:05:00Z",
}));
auto out = events.tl().ceil("H");
std::cout << out;
```

### Output

```text
0  2024-06-01 10:00:00
1  2024-06-01 15:00:00

ptype: Timeline
```

Both rows advance to the start of the *next* hour, even though neither was
already on an hour boundary.

Example 2. A row already exactly on a boundary is left unchanged, not advanced
further.

```cpp
auto exact = Thread(pluss::make_timeline({"2024-06-01T14:05:01.000000000Z"}));
auto out2 = exact.tl().ceil("S");
std::cout << out2;
```

### Output

```text
0  2024-06-01 14:05:01

ptype: Timeline
```

Example 3. An unsupported unit throws.

```cpp
try {
  auto bad = events.tl().ceil("week");
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

- A row that already sits exactly on a `unit` boundary is left unchanged (not
  advanced to the *next* boundary).
- Ceiling happens in **local wall time** (per the thread's current timezone),
  then maps back.
- `unit` cannot be empty, and only accepts the fixed clock units listed above.
- `.tl()` requires the bound thread to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row stays `NaT` (displays as `"NULL"`) regardless of `unit`.

## See Also

- [`ThreadTimelineAccessor::floor`](floor.md),
  [`ThreadTimelineAccessor::round`](round.md): round down, or to the nearest
  boundary, instead of always up.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="date.html">
    <span class="label">Next</span>
    <span class="title">date</span>
  </a>
</div>
