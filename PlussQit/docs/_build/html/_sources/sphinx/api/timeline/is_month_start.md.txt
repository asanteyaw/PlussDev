# Timeline::is_month_start

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_month_start() const
```

## What Counts As A Month Start

`is_month_start()` marks every instant whose local day-of-month is `1`.
Time-of-day is deliberately irrelevant: midnight, noon, and the final
nanosecond before day 2 all belong to the first calendar date. The method does
not inspect ordering or neighbors, so “start” never means the first stored
observation or the earliest instant in the receiver.

This is a civil-calendar predicate. It does not move an absent first-of-month
date into the data, choose the first weekday, or apply a holiday calendar.

## Result Contract

The method returns a new boolean `Series`, preserving rank and every extent.
`NaT` becomes `false`; zero-length dimensions remain zero-length. The receiver
and its nanosecond values are unchanged. Calendar fields are interpreted in
the Timeline's current timezone.

## Example 1: Calendar Membership

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_month_start());
```

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[false, true, false, false, true, false, true, false, false]
```

## Example 2: Matrices Stay Matrices

```cpp
const auto dates = pluss::timeline::make({
    {"2024-01-01T23:59:59.999999999Z", "2024-01-02T00:00:00Z",
     "2024-02-29T00:00:00Z"},
    {"2024-03-31T12:00:00Z", "2024-04-01T00:00:00Z",
     "2024-12-31T00:00:00Z"},
});
pluss::series::print(std::cout, dates.is_month_start());
```

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡  true false false ⎤
⎣ false  true false ⎦
```

The late time on January 1 remains a month start. No flattening or reduction
occurs.

## Example 3: Timezone Chooses The Local Date

```cpp
const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
const auto local = utc.tz_convert("America/New_York");
std::cout << std::boolalpha
          << "UTC=" << utc.is_month_start().item<bool>() << '\n'
          << "New York=" << local.is_month_start().item<bool>() << '\n';
```

```text
UTC=true
New York=false
```

## Example 4: Empty Geometry Is Preserved

```cpp
const auto scalar = pluss::timeline::make("2024-02-29T00:00:00Z");
const auto empty = pluss::timeline::full({0}, scalar);
pluss::series::print(std::cout, empty.is_month_start());
```

```text
Series(shape=[0], strides=[1], dtype=bool, platform=cpu, numel=0, contiguous=true)
[]
```

## Relationships And Failure Modes

Every calendar quarter start and year start is also a month start, but most
month starts are neither. `false` also covers `NaT`; pair with `isvalid()` when
that distinction matters. There are no value-domain failures for a valid
Timeline and no scalar-rank restriction. Use `month_start_scalar()` only when
the receiver is rank zero and a native `bool` is required.

## See Also

- [`month_start_scalar`](month_start_scalar.md)
- [`is_month_end`](is_month_end.md)
- [`is_quarter_start`](is_quarter_start.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_month_end.html"><span class="label">Previous</span><span class="title">is_month_end</span></a>
  <a class="next-link" href="is_quarter_end.html"><span class="label">Next</span><span class="title">is_quarter_end</span></a>
</div>
<!-- pluss-hand-authored -->
