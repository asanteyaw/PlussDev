<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3e01009d3a85 -->

# DataFrameTimelineAccessor::localize

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::localize(const TimelineLocalizeOptions& options = {}) const -> DataFrame
```

## What It Does

Reinterprets every selected `Timeline` feature's *current* clock
reading as a naive local wall-clock time in `options.tz`, and converts it to
the true UTC instant that local reading corresponds to — same feature name,
still `Timeline`-kind, same row order. **This changes the
underlying instant** (unlike [`tz()`](tz.md), which only relabels display).
The row is then displayed under `options.tz`, so its wall-clock text looks
unchanged even though the instant it now represents has shifted.

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

`DataFrame`
    A new frame with each selected time feature's underlying instant
    recomputed as if its prior reading were local time in `options.tz`.

## Examples

Example 1. Localize naive-looking readings to `"America/New_York"`.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Slate(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-07-15T09:15:30Z",
      }))}});
std::cout << events;

TimelineLocalizeOptions options;
options.tz = "America/New_York";
auto out = events.tl().localize(options);
std::cout << out;

auto before_epoch = events.tl().epoch(TimeUnit::Seconds);
auto after_epoch = out.tl().epoch(TimeUnit::Seconds);
std::cout << before_epoch;
std::cout << after_epoch;
```

### Output (before)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-07-15 09:15:30
--------------------------
[2 cs x 1 feat]
```

### Output (after localize)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-07-15 09:15:30
--------------------------
[2 cs x 1 feat]
```

### Output (epoch seconds, before)

```text
=================
     | ts        
-----------------
0    | 1706692530
1    | 1721034930
-----------------
[2 cs x 1 feat]
```

### Output (epoch seconds, after localize)

```text
=================
     | ts        
-----------------
0    | 1706710530
1    | 1721049330
-----------------
[2 cs x 1 feat]
```

The displayed wall-clock text is identical before and after — both still read
`09:15:30` — because the result is displayed under the new
`"America/New_York"` label, which reintroduces the same offset that was just
subtracted. The `epoch(Seconds)` values prove the underlying instant actually
moved: `18,000` seconds later in January (`5` hours, Eastern Standard Time)
and `14,400` seconds later in July (`4` hours, Eastern Daylight Time).

Example 2. Use `fold` to disambiguate a wall-clock reading that occurs twice
during New York's fall-back transition.

```cpp
using namespace pluss::table;

auto reading = DataFrame(
    {{"ts", Slate(pluss::make_timeline({"2024-11-03T01:30:00Z"}))}});

TimelineLocalizeOptions fold0;
fold0.tz = "America/New_York";
fold0.fold = 0;
auto out_fold0 = reading.tl().localize(fold0);
std::cout << out_fold0.tl().epoch(TimeUnit::Seconds);

TimelineLocalizeOptions fold1;
fold1.tz = "America/New_York";
fold1.fold = 1;
auto out_fold1 = reading.tl().localize(fold1);
std::cout << out_fold1.tl().epoch(TimeUnit::Seconds);
```

### Output (fold = 0, epoch seconds)

```text
=================
     | ts        
-----------------
0    | 1730611800
-----------------
[1 cs x 1 feat]
```

### Output (fold = 1, epoch seconds)

```text
=================
     | ts        
-----------------
0    | 1730615400
-----------------
[1 cs x 1 feat]
```

`01:30` local occurs twice on `2024-11-03` — once while still on Eastern
Daylight Time (`-4:00`), once after falling back to Eastern Standard Time
(`-5:00`). `fold = 0` picks the earlier occurrence (the daylight-time one);
`fold = 1` picks the later occurrence, exactly `3,600` seconds after. Both
share the same displayed wall-clock text (`01:30:00`) but represent different
instants.

Example 3. Use `nonexistent = "shift_forward"` to handle a wall-clock reading
that falls in New York's spring-forward gap (`02:00`-`03:00` local on
transition day, which never actually occurs).

```cpp
using namespace pluss::table;

auto reading = DataFrame(
    {{"ts", Slate(pluss::make_timeline({"2024-03-10T02:30:00Z"}))}});

TimelineLocalizeOptions options;
options.tz = "America/New_York";
options.nonexistent = "shift_forward";
auto out = reading.tl().localize(options);
std::cout << out;
```

### Output

```text
==========================
     | ts                 
--------------------------
0    | 2024-03-10 03:00:00
--------------------------
[1 cs x 1 feat]
```

`02:30` local never happened on `2024-03-10` — clocks jumped straight from
`02:00` to `03:00`. `"shift_forward"` resolves the reading to the first
instant *after* the gap closes, `03:00:00`, rather than throwing.

Example 4. Leave `nonexistent` at its default `"raise"` for the same
spring-forward-gap reading, to see it throw instead.

```cpp
using namespace pluss::table;

auto reading = DataFrame(
    {{"ts", Slate(pluss::make_timeline({"2024-03-10T02:30:00Z"}))}});

TimelineLocalizeOptions options;
options.tz = "America/New_York";

try {
  auto out = reading.tl().localize(options);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
tz_localize: nonexistent local time (DST spring-forward gap)
```

Without `"shift_forward"`, a reading that falls in the gap has no valid
instant to resolve to, so the call throws rather than silently guessing.

Example 5. Localize to a different supported zone, `"Europe/London"`, outside
any DST transition window.

```cpp
using namespace pluss::table;

auto reading = DataFrame(
    {{"ts", Slate(pluss::make_timeline({"2024-06-01T10:00:00Z"}))}});

TimelineLocalizeOptions options;
options.tz = "Europe/London";
auto out = reading.tl().localize(options);
std::cout << out;

auto before_epoch = reading.tl().epoch(TimeUnit::Seconds);
auto after_epoch = out.tl().epoch(TimeUnit::Seconds);
std::cout << before_epoch;
std::cout << after_epoch;
```

### Output (localized display)

```text
==========================
     | ts                 
--------------------------
0    | 2024-06-01 10:00:00
--------------------------
[1 cs x 1 feat]
```

### Output (epoch seconds, before)

```text
=================
     | ts        
-----------------
0    | 1717236000
-----------------
[1 cs x 1 feat]
```

### Output (epoch seconds, after)

```text
=================
     | ts        
-----------------
0    | 1717232400
-----------------
[1 cs x 1 feat]
```

`10:00:00` local in `Europe/London` on June 1st is British Summer Time
(`+1:00`), so the underlying instant moves `3,600` seconds *earlier* — the
opposite direction from the `America/New_York` examples above, since London's
offset is positive rather than negative.

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
  spring-forward-gap reading, regardless of what the string itself says.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row stays `NaT` regardless of `options`.

## See Also

- [`DataFrameTimelineAccessor::tz`](tz.md): relabels display timezone without
  changing the instant — the operation most people actually want.
- [`DataFrameTimelineAccessor::utc`](utc.md): shorthand for `tz("UTC")`.

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
