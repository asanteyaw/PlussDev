<!-- pluss-hand-authored -->

# TimeIndex API Reference

[Back to PlussQit API Reference](../api_reference.md)

This page documents `pluss::TimeIndex` and `pluss::codeh::TimeIndex` in the same
handwritten style as the `Series` reference: one method page at a time, with
usage-first prose, practical examples, and direct notes about the parts that
matter in real time-series code.

## What `TimeIndex` Is

`TimeIndex` is the index-oriented temporal object in PlussQit.

Use it when your timestamps are not just "time values" but the case axis you
intend to search, slice, align, resample, and join against.

That is the main difference from `Timeline`:

- `Timeline` is the general temporal payload. It can be scalar or N-D.
- `TimeIndex` is the 1D time axis surface. It is where ordered lookup semantics
  live.

Construction is intentionally routed through `pluss::timeindex::make(...)` or
`Timeline::to_timeindex(...)`, not through public constructors.

## Includes

```cpp
#include <pluss/pluss.h>
// or the narrower surface
#include <pluss/codeh/codeh.h>
#include <pluss/codeh/timeindex_factory.h>
```

## Practical Construction Patterns

Example 1. Build directly from ISO-like timestamp text.

```cpp
auto idx = pluss::timeindex::make({
    "2026-01-05T09:00:00Z",
    "2026-01-05T09:15:00Z",
    "2026-01-05T09:45:00Z",
    "2026-01-05T10:05:00Z",
});
```

Example 2. Start from a `Timeline`, then promote it into index semantics.

```cpp
auto axis = pluss::timeline::make({
    "2026-01-05T09:00:00Z",
    "2026-01-05T09:15:00Z",
    "2026-01-05T09:45:00Z",
    "2026-01-05T10:05:00Z",
});

auto idx = pluss::timeindex::make(axis);
```

Example 3. Build from raw epoch nanoseconds when you already have machine-time
data.

```cpp
std::vector<std::int64_t> ticks{
    1'767'904'800'000'000'000LL,
    1'767'905'700'000'000'000LL,
    1'767'907'500'000'000'000LL,
};

auto idx = pluss::timeindex::make(ticks);
```

## Reading ISO Timestamps: Why `T` and `Z` Appear

Pluss accepts ISO-8601-style timestamp text, so you will often see forms like:

```text
2026-01-05T09:15:00Z
```

- `T` is the standard separator between the date portion and the time portion.
- `Z` means UTC, also called "Zulu" time.

Other suffix styles are also common:

- `2026-01-05T09:15:00+00:00`
  Equivalent to UTC, just written as an explicit numeric offset.
- `2026-01-05T09:15:00-07:00`
  A timestamp with a negative timezone offset.
- `2026-01-05`
  A date-only form. Parsers usually interpret this as midnight for that date.
- `2026-01-05T09:15:00`
  A timestamp with no explicit timezone suffix. Whether this is treated as naive
  or localized depends on the parser and call path, so explicit offsets are
  safer when timezone correctness matters.

## Using This Reference

- Use the left sidebar to open one `TimeIndex` method page at a time.
- The first pass focuses on the methods that make `TimeIndex` different from a
  generic `Timeline`: searching, slicing, resampling, rolling windows, and
  time-aware alignment.
- If you need broader temporal transforms such as formatting, timezone
  conversion, or field extraction, start with
  [`TimeIndex::timeline`](api/timeindex/timeline.md) and then continue on the
  returned `Timeline`.

```{toctree}
:hidden:
:maxdepth: 1

api/timeindex/timeline
api/timeindex/size
api/timeindex/freq
api/timeindex/host
api/timeindex/is_monotonic_increasing
api/timeindex/is_unique
api/timeindex/min
api/timeindex/max
api/timeindex/sort
api/timeindex/unique
api/timeindex/argsort
api/timeindex/take
api/timeindex/slice_index
api/timeindex/head
api/timeindex/tail
api/timeindex/searchsorted
api/timeindex/slice
api/timeindex/slice_time
api/timeindex/asof
api/timeindex/union_
api/timeindex/intersection
api/timeindex/difference
api/timeindex/floor
api/timeindex/ceil
api/timeindex/round
api/timeindex/normalize
api/timeindex/to_note_iso
api/timeindex/tz_convert
api/timeindex/tz_localize
api/timeindex/resample_bins
api/timeindex/resample_count
api/timeindex/resample_sum
api/timeindex/resample_mean
api/timeindex/resample_agg
api/timeindex/resample_ohlc
api/timeindex/rolling_count
api/timeindex/rolling_sum
api/timeindex/rolling_agg
api/timeindex/join_asof
api/timeindex/align_union
```

## Coverage

The current handwritten pass now covers the canonical `TimeIndex` surface:

- introspection and cadence: `size`, `freq`, `host`
- ordering and uniqueness checks: monotonic predicates, uniqueness predicates
- extrema and ordering transforms: `min`, `max`, `sort`, `unique`, `argsort`
- positional selection: `take`, `slice_index`, `head`, `tail`
- ordered lookup: `searchsorted`, `asof`
- label-based slicing: `slice`, `slice_time`
- set-style axis operations: `union_`, `intersection`, `difference`
- axis rounding and formatting: `floor`, `ceil`, `round`, `normalize`,
  `to_note_iso`
- timezone handling: `tz_convert`, `tz_localize`
- resampling primitives: `resample_bins`, `resample_count`, `resample_sum`,
  `resample_mean`, `resample_agg`, `resample_ohlc`
- rolling window primitives: `rolling_count`, `rolling_sum`, `rolling_agg`
- alignment and as-of joins: `join_asof`, `align_union`

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
  <a class="next-link" href="api/timeindex/timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
