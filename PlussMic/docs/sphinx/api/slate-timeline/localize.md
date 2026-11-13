<!-- pluss-hand-authored -->

# SlateTimelineAccessor::localize

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
Slate SlateTimelineAccessor::localize(const TimelineLocalizeOptions& options = {}) const
```

## What It Does

Reinterprets the bound slate's *current* clock reading as a naive local
wall-clock time in `options.tz`, and converts it to the true UTC instant that
local reading corresponds to — same length, still `Timeline`-kind.
**This changes the underlying instant** (unlike [`tz()`](tz.md), which only
relabels display). The result is then displayed under `options.tz`, so its
wall-clock text looks unchanged even though the instant it now represents has
shifted.

## Parameters

**options** (`const TimelineLocalizeOptions&`)

`tz` (default `"UTC"`; must be one of the four supported names — see
[`tz()`](tz.md)'s Notes — and cannot be empty), `fold` (`0` or `1`; during the
one-hour daylight-saving *fall-back* overlap, `fold = 0` resolves to the
daylight offset and `fold = 1` resolves to the standard offset), and
`nonexistent` (only the literal `"shift_forward"` is recognized as a real
handling mode; any other value — including the default `"raise"` — throws
when a row's local reading falls in the daylight-saving *spring-forward* gap,
a wall-clock reading that never actually occurred).

## Returns

`Slate`
    A new `Timeline`-kind slate, same length as the bound slate,
    with its underlying instant recomputed as if its prior reading were local
    time in `options.tz`.

## Examples

Example 1. Localize a naive-looking reading to `"America/New_York"` and
confirm the underlying instant moved via `epoch()`.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-07-15T09:15:30Z",
}));

TimelineLocalizeOptions options;
options.tz = "America/New_York";
auto out = events.tl().localize(options);
std::cout << out;

std::cout << events.tl().epoch(TimeUnit::Seconds);
std::cout << out.tl().epoch(TimeUnit::Seconds);
```

### Output (localized display)

```text
0  2024-01-31 09:15:30
1  2024-07-15 09:15:30

ptype: Timeline
```

### Output (epoch seconds, before)

```text
0  1706692530
1  1721034930

ptype: Series
```

### Output (epoch seconds, after)

```text
0  1706710530
1  1721049330

ptype: Series
```

The displayed wall-clock text is identical before and after — both still read
`09:15:30` — because the result is displayed under the new
`"America/New_York"` label, which reintroduces the same offset that was just
subtracted. The `epoch(Seconds)` values prove the underlying instant actually
moved: `18,000` seconds later in January (Eastern Standard Time) and `14,400`
seconds later in July (Eastern Daylight Time).

Example 2. Use `fold` to disambiguate a wall-clock reading that occurs twice
during New York's fall-back transition.

```cpp
auto reading = Slate(pluss::make_timeline({"2024-11-03T01:30:00Z"}));

TimelineLocalizeOptions fold0;
fold0.tz = "America/New_York";
fold0.fold = 0;
std::cout << reading.tl().localize(fold0).tl().epoch(TimeUnit::Seconds);

TimelineLocalizeOptions fold1;
fold1.tz = "America/New_York";
fold1.fold = 1;
std::cout << reading.tl().localize(fold1).tl().epoch(TimeUnit::Seconds);
```

### Output (fold = 0)

```text
0  1730611800

ptype: Series
```

### Output (fold = 1)

```text
0  1730615400

ptype: Series
```

`01:30` local occurs twice on `2024-11-03` — `fold = 0` picks the earlier
(daylight-time) occurrence; `fold = 1` picks the later one, exactly `3,600`
seconds after.

Example 3. `nonexistent = "shift_forward"` resolves a spring-forward-gap
reading instead of throwing.

```cpp
auto gap = Slate(pluss::make_timeline({"2024-03-10T02:30:00Z"}));

TimelineLocalizeOptions options;
options.tz = "America/New_York";
options.nonexistent = "shift_forward";
auto out3 = gap.tl().localize(options);
std::cout << out3;
```

### Output

```text
0  2024-03-10 03:00:00

ptype: Timeline
```

`02:30` local never happened on `2024-03-10` — clocks jumped straight from
`02:00` to `03:00`. `"shift_forward"` resolves to the first instant *after*
the gap closes.

Example 4. Leave `nonexistent` at its default `"raise"` for the same reading,
to see it throw.

```cpp
TimelineLocalizeOptions defaults;
defaults.tz = "America/New_York";
try {
  auto bad = gap.tl().localize(defaults);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
tz_localize: nonexistent local time (DST spring-forward gap)
```

## Notes

- **`localize()` changes the underlying instant; [`tz()`](tz.md) never
  does.** Reach for `localize()` only when the current ticks are naive local
  readings that have never been anchored to a real instant; reach for `tz()`
  when the ticks are already correct and you just want a different display
  timezone.
- `options.tz` only accepts the same four names as `tz()` — anything else
  throws — and cannot be empty.
- `fold` only matters for the one hour of wall-clock time that repeats during
  a fall-back transition; outside that window it has no effect.
- `nonexistent` only recognizes `"shift_forward"` as an actual handling
  strategy; every other value (default `"raise"` included) throws for a
  spring-forward-gap reading.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row stays `NaT` regardless of `options`.

## See Also

- [`SlateTimelineAccessor::tz`](tz.md): relabels display timezone without
  changing the instant — the operation most people actually want.
- [`SlateTimelineAccessor::utc`](utc.md): shorthand for `tz("UTC")`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="leap.html">
    <span class="label">Previous</span>
    <span class="title">leap</span>
  </a>
  <a class="next-link" href="minute.html">
    <span class="label">Next</span>
    <span class="title">minute</span>
  </a>
</div>
