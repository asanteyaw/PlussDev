# Timeline (`tl`) and Batched DataFrame Operations

This page covers the Pluss-native timeline surface and the batched numeric
delegation path added for multi-feature DataFrame operations.

## API Signature

`Thread::tl() -> ThreadTimelineAccessor`  
`DataFrame::tl(feature: string_view) -> DataFrameTimelineAccessor`  
`DataFrame::tl(features: initializer_list<string_view>) -> DataFrameTimelineAccessor`  
`DataFrame::tl(features: RangeView<string_view> = {}) -> DataFrameTimelineAccessor`  
`Atlas::tl(feature: string_view) -> AtlasTimelineAccessor`  
`Atlas::tl(features: initializer_list<string_view>) -> AtlasTimelineAccessor`  
`Atlas::tl(features: RangeView<string_view> = {}) -> AtlasTimelineAccessor`

## Notes

`tl()` is the datetime/timeline surface. It is separate from `ts()`: `tl()`
extracts and transforms datetime payloads, while `ts()` remains the time-series
analytics surface for returns, lag/lead, rolling blocks, resampling, and
alignment.

## Inputs

- `feature`: one Timeline or TimeIndex feature name.
- `features`: zero or more Timeline/TimeIndex feature names. Empty selection
  is valid only after an intentional selection where every feature in the
  current frame is Timeline/TimeIndex, for example
  `df.features({"event_time"}).tl()`.
- `unit`: calendar/time unit accepted by `Timeline` rounding methods, such as
  `"D"`, `"H"`, `"min"`, or compatible CodeH units.
- `periods`: signed number of frequency steps to shift.
- `freq`: shift/period frequency such as `"D"`, `"H"`, `"month"`, or
  `"year"`.
- `TimelineLocalizeOptions`: option struct with `.tz`, `.fold`, and
  `.nonexistent` fields.

## Output

- Thread-level `tl()` returns a `Thread`.
- DataFrame-level `tl()` returns a `DataFrame` with selected features replaced
  by the transformed result.
- Atlas-level `tl()` returns an `Atlas` after applying the DataFrame operation
  to each frame.

## Options

`TimeUnit` controls epoch extraction:

- `TimeUnit::Nanos`
- `TimeUnit::Micros`
- `TimeUnit::Millis`
- `TimeUnit::Seconds`

`TimelineLocalizeOptions` supports C++20 designated initializers:

```cpp
TimelineLocalizeOptions{.tz = "UTC", .fold = 0, .nonexistent = "raise"}
```

## Examples

### Example 1: Thread timeline extraction

```cpp
auto years = df.features("event_time").tl().year();
auto iso = df.features("event_time").tl().iso();
auto seconds = df.features("event_time").tl().epoch(TimeUnit::Seconds);
```

Expected output:

- `years` is a numeric Thread containing calendar years.
- `iso` is a Note Thread containing formatted ISO-like datetime strings.
- `seconds` is a numeric Thread containing epoch seconds.

### Example 2: Thread reduction preserves timeline payload

```cpp
auto first_time = df.features("event_time").min();
auto last_time = df.features("event_time").max();
```

Expected output:

- `first_time` is a `ThreadValue` wrapping a one-element Timeline payload.
- `last_time` is a `ThreadValue` wrapping a one-element Timeline payload.

### Example 3: DataFrame timeline transformation

```cpp
auto year_frame = df.tl("event_time").year()
    .rename({{"event_time", "event_year"}});

auto rounded = df.tl("event_time").floor("D");
auto monthly = df.tl("event_time").period("month");
```

Expected output:

- `year_frame` keeps the original frame shape and replaces `event_time` with
  the extracted year, renamed to `event_year`.
- `rounded` keeps `event_time` as a Timeline/TimeIndex feature rounded down to
  day precision.
- `monthly` keeps `event_time` as a Timeline/TimeIndex feature bucketed to the
  start of its calendar month.

### Example 4: Option-struct localization

```cpp
auto localized = df.tl("event_time").localize(
    TimelineLocalizeOptions{.tz = "UTC", .fold = 0, .nonexistent = "raise"});
```

Expected output:

- `localized` is a DataFrame with `event_time` localized using the supplied
  timezone rules.

### Example 5: Atlas-level timeline delegation

```cpp
Atlas atlas({jan, feb});
auto months = atlas.tl("event_time").month();
```

Expected output:

- `months` is an Atlas with each frame transformed through
  `DataFrame::tl("event_time").month()`.

### Example 6: Batched numeric DataFrame math

```cpp
auto scaled = df.features({"sales", "expense", "margin"}).mul(2.0);
auto compared = df.features({"sales", "expense", "margin"}).gt(20.0);
auto combined = df.features({"sales", "expense", "margin"})
    .add(scaled.features({"sales", "expense", "margin"}));
```

Expected output:

- `scaled` is a DataFrame where compatible numeric features are packed into a
  matrix batch, multiplied once, and unpacked.
- `compared` is a boolean DataFrame using the same batched scalar comparison
  path where dtype/platform compatibility allows it.
- `combined` uses the DataFrame-vs-DataFrame batched pair path for compatible
  numeric feature groups, falling back per Thread for mixed payloads.
