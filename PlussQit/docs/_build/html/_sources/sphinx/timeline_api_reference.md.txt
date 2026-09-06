<!-- pluss-hand-authored -->

# Timeline API Reference

[Back to PlussQit API Reference](../api_reference.md)

This page documents `pluss::Timeline` one method at a time. Each method page
is handwritten, navigable from the sidebar, and backed by compiled examples
whose displayed output comes from the implementation.

## What `Timeline` Is

`Timeline` is the general temporal payload object in PlussQit.

Use it when timestamps are part of the data itself:

- a scalar moment
- a 1D column of event times
- a 2D or 3D temporal tensor
- date-part extraction
- text formatting of timestamps
- timezone conversion
- mask-based filling and temporal selection

Use [`TimeIndex`](timeindex_api_reference.md) instead when the timestamps are
the ordered case axis and you specifically want ordered lookup, as-of matching,
resampling, rolling windows, and alignment semantics.

That difference is the clean split:

- `Timeline` is the general time payload.
- `TimeIndex` is the 1D ordered time axis surface built on top of a
  `Timeline`.

## Includes

```cpp
#include <pluss/pluss.h>
```

## Practical Construction Patterns

Example 1. Build a scalar timestamp.

```cpp
auto t = pluss::timeline::make("2026-03-14T09:26:53Z");
```

Example 2. Build a 1D temporal vector.

```cpp
auto t = pluss::timeline::make({
    "2026-03-14T09:00:00Z",
    "2026-03-14T10:00:00Z",
    "2026-03-14T11:00:00Z",
});
```

Example 3. Build a higher-rank temporal array.

```cpp
auto t = pluss::timeline::make({
    {"2026-03-14T09:00:00Z", "2026-03-14T10:00:00Z"},
    {"2026-03-15T09:00:00Z", "2026-03-15T10:00:00Z"},
});
```

Example 4. Build directly from raw epoch nanoseconds.

```cpp
auto t = pluss::timeline::from_epoch({
    1'773'484'800'000'000'000LL,
    1'773'488'400'000'000'000LL,
});
```

## Reading ISO-like Timestamps

Pluss accepts ISO-8601-style timestamp text, so you will often see forms like:

```text
2026-03-14T09:26:53Z
```

- `T`
  Separates the date portion from the time portion.
- `Z`
  Means UTC, also called Zulu time.

Other common forms are also valid when the parser can unambiguously interpret
them:

- `2026-03-14T09:26:53+00:00`
  UTC written as an explicit numeric offset.
- `2026-03-14T09:26:53-07:00`
  A timestamp with a negative timezone offset.
- `2026-03-14`
  A date-only form, usually interpreted as midnight on that date.

Important current behavior:

- The documented text path for a missing timestamp is an empty string, not the
  literal text `"NaT"`.
- `Timeline` may contain missing times, and those render as `NaT`.

## Coverage

The current handwritten pages cover these starting families:

- representation and export: `defined`, `sizes`, `dim`, `numel`,
  `is_scalar`, `tz`, `host`, `to_epoch_nanos_vector`,
  `to_epoch_nanos_flat`, `epoch_nanos`
- arithmetic and distance: `add_duration`, `sub_duration`, `diff`, `add_nanos`
- comparisons: `eq`, `ne`, `lt`, `le`, `gt`, `ge`
- missing-time work: `isnat`, `isvalid`, `isnat_scalar`, `fill_nat`, `where`
- ordering and deduplication: `argsort`, `min`, `max`, `sort`, `unique`,
  `drop_duplicates`
- scalar formatting: `isoformat`
- formatting and text conversion: `strftime`, `to_note_iso`, the convenience
  `to_note_*` family, `weekday_name`, `month_name`
- rounding and projection: `floor`, `ceil`, `round`, `normalize`, `date`,
  `time`
- timezone work: `tz_convert`, `tz_localize`, `to_utc`
- ordered insertion helpers on sorted 1D timelines:
  `searchsorted_left`, `searchsorted_right`
- calendar component extraction: `year`, `month`, `day`, `hour`, `minute`,
  `second`, `nanosecond`, `microsecond`, `millisecond`, `weekday`,
  `dayofyear`, `quarter`, `week`
- calendar predicates: `is_leap_year`, boundary predicates, business-day
  predicates, and the weekday-specific predicates
- regular sequence construction: `date_range(...)`

This is not yet the full public surface. The representation, export, canonical
calendar-field, and calendar-predicate layers are now documented method by
method; later passes will separate ordering operations and transformations
that are still grouped behind their starting pages. Public aliases are listed
on their canonical method page so readers can recognize every supported
spelling.

## Using This Reference

- Use the left sidebar to open one `Timeline` method page at a time.
- Each page now embeds verified output directly under the example that produced
  it.
- Alias pages identify the canonical method they forward to and explain when
  the alternate spelling is useful; they do not replace method-level
  navigation.

```{toctree}
:hidden:
:maxdepth: 1

api/timeline/defined
api/timeline/sizes
api/timeline/dim
api/timeline/numel
api/timeline/is_scalar
api/timeline/tz
api/timeline/host
api/timeline/to_epoch_nanos_vector
api/timeline/to_epoch_nanos_flat
api/timeline/epoch_nanos
api/timeline/add_duration
api/timeline/sub_duration
api/timeline/diff
api/timeline/eq
api/timeline/isnat
api/timeline/isvalid
api/timeline/isnat_scalar
api/timeline/argsort
api/timeline/isoformat
api/timeline/strftime
api/timeline/to_note_iso
api/timeline/to_note_family
api/timeline/weekday_name
api/timeline/month_name
api/timeline/floor
api/timeline/ceil
api/timeline/round
api/timeline/normalize
api/timeline/tz_convert
api/timeline/tz_localize
api/timeline/to_utc
api/timeline/min
api/timeline/fill_nat
api/timeline/where
api/timeline/add_nanos
api/timeline/searchsorted_left
api/timeline/year
api/timeline/month
api/timeline/day
api/timeline/hour
api/timeline/minute
api/timeline/second
api/timeline/nanosecond
api/timeline/microsecond
api/timeline/millisecond
api/timeline/weekday
api/timeline/dayofyear
api/timeline/quarter
api/timeline/week
api/timeline/is_leap_year
api/timeline/is_month_start
api/timeline/is_month_end
api/timeline/is_quarter_start
api/timeline/is_quarter_end
api/timeline/is_year_start
api/timeline/is_year_end
api/timeline/is_weekend
api/timeline/is_business_day
api/timeline/is_monday
api/timeline/is_tuesday
api/timeline/is_wednesday
api/timeline/is_thursday
api/timeline/is_friday
api/timeline/is_saturday
api/timeline/is_sunday
api/timeline/date_range
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
  <a class="next-link" href="api/timeline/defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
