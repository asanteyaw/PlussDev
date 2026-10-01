<!-- pluss-hand-authored -->

# Timeline Index Accessor API Reference

[Back to PlussQit API Reference](../api_reference.md)

This is the method reference for the ordered-axis workflow entered through
[`Timeline::index()`](api/timeline/index.md). Each operation retains the
existing verified TimeIndex teaching while presenting its canonical home on
`Timeline`.

## What `TimelineIndexAccessor` Is

`TimelineIndexAccessor` is a lightweight checked view of a one-dimensional
`Timeline`. It owns a cheap Timeline handle, so it is safe to retain after the
expression that created it. Building the accessor validates rank and absent
instants and, by default, scans once for nondecreasing order. Calls made through
the retained accessor reuse that validated state.

Use it when your timestamps are not just "time values" but the case axis you
intend to search, slice, align, resample, and join against.

That is the main difference from `Timeline`:

- `Timeline` is the general temporal payload. It can be scalar or N-D.
- `TimelineIndexAccessor` is the 1D time axis surface. It is where ordered lookup semantics
  live.

Enter this surface with `timeline.index()`. During the staged SDK migration,
`TimelineIndexAccessor` is a source-level name for the existing public
`TimeIndex` implementation. That preserves current binaries and downstream
source while new code adopts the accessor spelling. The compatibility
factories remain available temporarily, but they do not define a second set of
index semantics.

## Includes

```cpp
#include <pluss/pluss.h>
// or the narrower surface
#include <pluss/codeh/codeh.h>
```

## Practical Construction Patterns

Example 1. Validate a Timeline as an ordered axis.

```cpp
auto timeline = pluss::timeline::make({
    "2026-01-05T09:00:00Z",
    "2026-01-05T09:15:00Z",
    "2026-01-05T09:45:00Z",
    "2026-01-05T10:05:00Z",
});
auto idx = timeline.index();
```

Example 2. Retain one accessor when several operations share the same axis.

```cpp
auto axis = pluss::timeline::make({
    "2026-01-05T09:00:00Z",
    "2026-01-05T09:15:00Z",
    "2026-01-05T09:45:00Z",
    "2026-01-05T10:05:00Z",
});

auto idx = axis.index();
auto insertion = idx.searchsorted(
    pluss::timeline::make("2026-01-05T09:30:00Z"));
auto previous = idx.asof(
    pluss::timeline::make("2026-01-05T09:30:00Z"));
```

Example 3. Skip the ordering scan only when the next operation does not depend
on sorted input.

```cpp
std::vector<std::int64_t> ticks{
    1'767'904'800'000'000'000LL,
    1'767'905'700'000'000'000LL,
    1'767'907'500'000'000'000LL,
};

auto axis = pluss::timeline::from_epoch(ticks, {3});
auto unchecked = axis.index(false);
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

- Use the left sidebar to open one `TimelineIndexAccessor` method page at a time.
- The method pages cover what makes the accessor different from a general
  `Timeline`: searching, slicing, resampling, rolling windows, and time-aware
  alignment.
- If you need broader temporal transforms such as formatting, timezone
  conversion, or field extraction, start with
  [`TimelineIndexAccessor::timeline`](api/timeline-index/timeline.md) and then continue on the
  returned `Timeline`.

```{toctree}
:hidden:
:maxdepth: 1

api/timeline-index/align_union
api/timeline-index/argsort
api/timeline-index/asof
api/timeline-index/ceil
api/timeline-index/difference
api/timeline-index/floor
api/timeline-index/freq
api/timeline-index/head
api/timeline-index/host
api/timeline-index/intersection
api/timeline-index/is_monotonic_decreasing
api/timeline-index/is_monotonic_increasing
api/timeline-index/is_unique
api/timeline-index/join_asof
api/timeline-index/max
api/timeline-index/min
api/timeline-index/operators
api/timeline-index/resample_agg
api/timeline-index/resample_bins
api/timeline-index/resample_count
api/timeline-index/resample_first
api/timeline-index/resample_last
api/timeline-index/resample_max
api/timeline-index/resample_mean
api/timeline-index/resample_min
api/timeline-index/resample_ohlc
api/timeline-index/resample_sum
api/timeline-index/rolling_agg
api/timeline-index/rolling_count
api/timeline-index/rolling_first
api/timeline-index/rolling_last
api/timeline-index/rolling_max
api/timeline-index/rolling_mean
api/timeline-index/rolling_min
api/timeline-index/rolling_sum
api/timeline-index/round
api/timeline-index/searchsorted
api/timeline-index/size
api/timeline-index/slice
api/timeline-index/slice_index
api/timeline-index/slice_time
api/timeline-index/sort
api/timeline-index/tail
api/timeline-index/take
api/timeline-index/timeline
api/timeline-index/to_note_iso
api/timeline-index/tz_convert
api/timeline-index/tz_localize
api/timeline-index/union_
api/timeline-index/unique
```

## Coverage

The current handwritten pass now covers the canonical `TimelineIndexAccessor` surface:

- introspection and cadence: `size`, `freq`, `host`
- ordering and uniqueness checks: monotonic predicates, uniqueness predicates
- extrema and ordering transforms: `min`, `max`, `sort`, `unique`, `argsort`
- positional selection: `take`, `slice_index`, `head`, `tail`
- ordered lookup: `searchsorted`, `asof`
- label-based slicing: `slice`, `slice_time`
- set-style axis operations: `union_`, `intersection`, `difference`
- axis rounding and formatting: `floor`, `ceil`, `round`,
  `to_note_iso`
- timezone handling: `tz_convert`, `tz_localize`
- resampling primitives: `resample_bins`, `resample_count`, `resample_first`,
  `resample_last`, `resample_max`, `resample_mean`, `resample_min`,
  `resample_sum`, `resample_agg`, `resample_ohlc`
- rolling window primitives: `rolling_count`, `rolling_first`, `rolling_last`,
  `rolling_max`, `rolling_mean`, `rolling_min`, `rolling_sum`, `rolling_agg`
- alignment and as-of joins: `join_asof`, `align_union`

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
  <a class="next-link" href="api/timeline-index/align_union.html">
    <span class="label">Next</span>
    <span class="title">align_union</span>
  </a>
</div>
