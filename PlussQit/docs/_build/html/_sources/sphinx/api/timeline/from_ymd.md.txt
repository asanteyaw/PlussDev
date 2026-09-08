<!-- pluss-hand-authored -->

# Timeline::from_ymd

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
static Timeline Timeline::from_ymd(
    const Series& year,
    const Series& month,
    const Series& day,
    std::string_view tz_name = "UTC")
```

## Assemble Local Calendar Dates

`from_ymd()` constructs local midnight from separate year, month, and day
components, then resolves each local value to an absolute instant in
`tz_name`. This differs fundamentally from epoch construction: the timezone
participates in determining the stored tick.

Each component may be scalar or have the common output shape. Scalars broadcast
to every position; non-scalar components must have exactly equal shapes.

```cpp
const auto year = pluss::make_series(2024LL);
const auto month = pluss::make_series({2LL, 3LL});
const auto day = pluss::make_series({29LL, pluss::Timeline::kNaT});
const auto dates = pluss::Timeline::from_ymd(year, month, day, "UTC");

std::cout << pluss::timeline::print(dates) << '\n';
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-02-29 00:00:00, NaT ]
```

An explicit `Timeline::kNaT` in any component makes that output position
`NaT`. An invalid represented date, such as 2024-02-30, throws instead of being
silently converted into absence. Month and day are one-based.

## Failure Modes

- Different non-scalar shapes are rejected; general array broadcasting is not
  performed.
- Invalid calendar fields throw and identify the logical position.
- An unknown timezone throws.
- Ranks above three are unsupported.

## See Also

- [`Timeline::from_ymdhms`](from_ymdhms.md)
- [`Timeline::date`](date.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_epoch_seconds.html"><span class="label">Previous</span><span class="title">from_epoch_seconds</span></a>
  <a class="next-link" href="from_ymdhms.html"><span class="label">Next</span><span class="title">from_ymdhms</span></a>
</div>
