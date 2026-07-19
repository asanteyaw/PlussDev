<!-- pluss-hand-authored -->

# DataFrameTimelineAccessor::tz

[Back to DataFrameTimelineAccessor API Reference](../../dataframe_timeline_accessor_api_reference)

## Signature

```cpp
DataFrameTimelineAccessor::tz(std::string_view name) const -> DataFrame
```

## What It Does

Replaces every selected `Timeline`/`TimeIndex` feature's *display* timezone
with `name` — same feature name, still `Timeline`/`TimeIndex`-kind, same row
order. **The underlying instant is unchanged** — only the timezone used to
compute local calendar fields (year/month/day/hour, and box-table display)
shifts.

## Parameters

**name** (`std::string_view`)

The target timezone. Only `"UTC"` (also the empty string), `"America/Edmonton"`,
`"America/New_York"`, and `"Europe/London"` are recognized (case-insensitive);
any other name throws.

## Returns

`DataFrame`
    A new frame with each selected time feature's display timezone switched
    to `name`.

## Examples

Example 1. Relabel a UTC `Timeline` feature to `"America/New_York"`.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-07-15T09:15:30Z",
      }))}});
std::cout << events;

auto out = events.tl().tz("America/New_York");
std::cout << out;
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

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 04:15:30
1    | 2024-07-15 05:15:30
--------------------------
[2 cs x 1 feat]
```

Both rows are the *same instants* as before — only their displayed local wall
time changes: `-5:00` in January (Eastern Standard Time) and `-4:00` in July
(Eastern Daylight Time), since `America/New_York`'s daylight-saving
transitions are calendar-aware.

Example 2. Relabel the same instants to `"America/Edmonton"`, one of the
other three supported names.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-07-15T09:15:30Z",
      }))}});
std::cout << events;

auto out = events.tl().tz("America/Edmonton");
std::cout << out;
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

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 02:15:30
1    | 2024-07-15 03:15:30
--------------------------
[2 cs x 1 feat]
```

`America/Edmonton` is `-7:00` in January (Mountain Standard Time) and `-6:00`
in July (Mountain Daylight Time) — a full hour further behind UTC than
`America/New_York` in both seasons.

Example 3. Relabel to `"Europe/London"`, where the winter reading matches UTC
exactly.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-07-15T09:15:30Z",
      }))}});
std::cout << events;

auto out = events.tl().tz("Europe/London");
std::cout << out;
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

### Output (after)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-07-15 10:15:30
--------------------------
[2 cs x 1 feat]
```

`Europe/London` sits at `+0:00` (GMT) in January, so that row's displayed
clock time doesn't change at all — only the row's timezone *label* changed,
which is easy to miss if you're only looking at the printed values. The July
row is `+1:00` (British Summer Time), so it does shift visibly.

Example 4. Relabel back to `"UTC"` to confirm `tz()` is fully reversible —
the original instants come back exactly.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({
          "2024-01-31T09:15:30Z",
          "2024-07-15T09:15:30Z",
      }))}});

auto ny = events.tl().tz("America/New_York");
std::cout << ny;

auto back_to_utc = ny.tl().tz("UTC");
std::cout << back_to_utc;
```

### Output (America/New_York)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 04:15:30
1    | 2024-07-15 05:15:30
--------------------------
[2 cs x 1 feat]
```

### Output (back to UTC)

```text
==========================
     | ts                 
--------------------------
0    | 2024-01-31 09:15:30
1    | 2024-07-15 09:15:30
--------------------------
[2 cs x 1 feat]
```

Relabeling to `America/New_York` and then straight back to `UTC` reproduces
the exact original display — confirming no instant was ever altered along the
way, only the lens used to read it.

Example 5. Pass an unrecognized timezone name to see the call throw.

```cpp
using namespace pluss::table;

auto events = DataFrame(
    {{"ts", Thread(pluss::make_timeline({"2024-01-31T09:15:30Z"}))}});

try {
  auto out = events.tl().tz("Asia/Tokyo");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Unknown timezone: Asia/Tokyo (currently supported: UTC, America/Edmonton, America/New_York, Europe/London)
```

`"Asia/Tokyo"` is a real IANA zone name, but this project only ships rules
for the four zones listed in the error — there's no general tzdata lookup
backing it.

## Notes

- Only four timezone names are supported: `"UTC"` (or empty string),
  `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` — any
  other name (e.g. `"Asia/Tokyo"`) throws. There is no general IANA tzdata
  lookup.
- **`tz()` never changes the underlying instant** — it only relabels which
  timezone is used to interpret it for display and field extraction (like
  [`hour()`](hour.md)/[`year()`](year.md)). Use [`localize()`](localize.md)
  if your data is naive local wall-clock text that needs to be *anchored* to
  a real instant for the first time.
- `DataFrame::tl()` requires **every** feature on the calling frame to already
  be `Timeline`/`TimeIndex`-kind — narrow with
  [`features()`](../dataframe/features.md) first if needed.
- A `NaT` row stays `NaT` regardless of `name`.

## See Also

- [`DataFrameTimelineAccessor::utc`](utc.md): shorthand for `tz("UTC")`.
- [`DataFrameTimelineAccessor::localize`](localize.md): reinterprets naive
  ticks as local time in a timezone, shifting the underlying instant —
  fundamentally different from this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="time.html">
    <span class="label">Previous</span>
    <span class="title">time</span>
  </a>
  <a class="next-link" href="utc.html">
    <span class="label">Next</span>
    <span class="title">utc</span>
  </a>
</div>
