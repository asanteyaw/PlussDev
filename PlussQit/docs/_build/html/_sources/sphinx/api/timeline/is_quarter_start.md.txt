# Timeline::is_quarter_start

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_quarter_start() const
```

## Calendar-Quarter Membership

`is_quarter_start()` marks January 1, April 1, July 1, and October 1 in the
Timeline's local calendar. It uses fixed January-based civil quarters. It does
not infer a fiscal calendar, inspect data ordering, or find the first observed
instant in a quarter.

Time-of-day is irrelevant. Every instant on a qualifying local date is marked,
including the final nanosecond of that date.

## Result Contract

The result is a new boolean `Series` with the receiver's exact rank and shape.
`NaT` becomes `false`; scalar and empty shapes remain scalar and empty. The
receiver is unchanged. Timezone conversion can alter the result because it
changes the local date used for classification, not because it changes the
stored instant.

## Example 1: Quarter Openings

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_quarter_start());
```

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[false, true, false, false, true, false, true, false, false]
```

January 1 is both a year and quarter start; April 1 starts a quarter but not a
year.

## Example 2: Higher-Rank Classification

```cpp
const auto dates = pluss::timeline::make({
    {"2024-01-01T23:59:59.999999999Z", "2024-01-02T00:00:00Z",
     "2024-02-29T00:00:00Z"},
    {"2024-03-31T12:00:00Z", "2024-04-01T00:00:00Z",
     "2024-12-31T00:00:00Z"},
});
pluss::series::print(std::cout, dates.is_quarter_start());
```

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡  true false false ⎤
⎣ false  true false ⎦
```

## Example 3: Timezone-Sensitive Boundary

```cpp
const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
const auto local = utc.tz_convert("America/New_York");
std::cout << std::boolalpha
          << "UTC=" << utc.is_quarter_start().item<bool>() << '\n'
          << "New York=" << local.is_quarter_start().item<bool>() << '\n';
```

```text
UTC=true
New York=false
```

## Example 4: Predicate Containment

```cpp
const auto starts = dates.is_quarter_start();
std::cout << std::boolalpha
          << starts.logical_and(dates.is_month_start()).equal(starts) << '\n';
```

```text
true
```

Every quarter start must be a month start. This identity is useful for checking
composed calendar masks.

## Gotchas And Failure Modes

- Fiscal quarters with another starting month require an explicit policy.
- `false` includes ordinary dates and `NaT`.
- The method supports every Timeline rank and does not reduce dimensions.
- Use `quarter_start_scalar()` for a native `bool` from rank-zero input.

## See Also

- [`quarter_start_scalar`](quarter_start_scalar.md)
- [`quarter`](quarter.md)
- [`is_quarter_end`](is_quarter_end.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_quarter_end.html"><span class="label">Previous</span><span class="title">is_quarter_end</span></a>
  <a class="next-link" href="is_saturday.html"><span class="label">Next</span><span class="title">is_saturday</span></a>
</div>
<!-- pluss-hand-authored -->
