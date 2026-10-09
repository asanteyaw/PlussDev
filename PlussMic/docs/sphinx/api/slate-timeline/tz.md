<!-- pluss-hand-authored -->

# SlateTimelineAccessor::tz

[Back to SlateTimelineAccessor API Reference](../../slate_timeline_accessor_api_reference)

## Signature

```cpp
SlateTimelineAccessor::tz(std::string_view name) const -> Slate
```

## What It Does

Replaces the bound slate's *display* timezone with `name` — same length,
still `Timeline`-kind. **The underlying instant is unchanged** —
only the timezone used to compute local calendar fields (year/month/day/hour,
and box-table display) shifts.

## Parameters

**name** (`std::string_view`)

The target timezone. Only `"UTC"` (also the empty string),
`"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` are
recognized (case-insensitive); any other name throws.

## Returns

`Slate`
    A new `Timeline`-kind slate, same length as the bound
    slate, with its display timezone switched to `name`.

## Examples

Example 1. Relabel a UTC slate to `"America/New_York"`.

```cpp
using namespace pluss::table;

auto events = Slate(pluss::make_timeline({
    "2024-01-31T09:15:30Z",
    "2024-07-15T09:15:30Z",
}));
auto out = events.tl().tz("America/New_York");
std::cout << out;
```

### Output

```text
0  2024-01-31 04:15:30
1  2024-07-15 05:15:30

ptype: Timeline
```

Both rows are the *same instants* as before — only their displayed local wall
time changes: `-5:00` in January (Eastern Standard Time) and `-4:00` in July
(Eastern Daylight Time).

Example 2. Relabel back to `"UTC"` to confirm `tz()` is fully reversible.

```cpp
auto back_to_utc = out.tl().tz("UTC");
std::cout << back_to_utc;
```

### Output

```text
0  2024-01-31 09:15:30
1  2024-07-15 09:15:30

ptype: Timeline
```

Relabeling to `America/New_York` and then straight back to `UTC` reproduces
the exact original display — confirming no instant was ever altered along the
way, only the lens used to read it.

Example 3. An unrecognized timezone name throws.

```cpp
try {
  auto bad = events.tl().tz("Asia/Tokyo");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Unknown timezone: Asia/Tokyo (currently supported: UTC, America/Edmonton, America/New_York, Europe/London)
```

`"Asia/Tokyo"` is a real IANA zone name, but this project only ships rules for
the four zones listed in the error — there's no general tzdata lookup
backing it.

## Notes

- Only four timezone names are supported: `"UTC"` (or empty string),
  `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` — any
  other name throws. There is no general IANA tzdata lookup.
- **`tz()` never changes the underlying instant** — it only relabels which
  timezone is used to interpret it for display and field extraction (like
  [`hour()`](hour.md)/[`year()`](year.md)). Use [`localize()`](localize.md) if
  the data is naive local wall-clock text that needs to be *anchored* to a
  real instant for the first time.
- `.tl()` requires the bound slate to already be `Timeline`-kind —
  the accessor's constructor throws otherwise.
- A `NaT` row stays `NaT` regardless of `name`.

## See Also

- [`SlateTimelineAccessor::utc`](utc.md): shorthand for `tz("UTC")`.
- [`SlateTimelineAccessor::localize`](localize.md): reinterprets naive ticks
  as local time in a timezone, shifting the underlying instant — fundamentally
  different from this method.

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
