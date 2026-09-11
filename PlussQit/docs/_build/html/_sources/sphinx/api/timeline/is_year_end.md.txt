# Timeline::is_year_end

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_year_end() const
```

## What Counts As A Year End

`is_year_end()` marks every instant on December 31 in the Timeline's local
civil calendar. It asks whether each instant belongs to the final calendar
date, not whether it is the largest value, the final array element, the last
observation available, or the final working day.

Time-of-day does not matter. Leap years still close on December 31; they only
contain one additional day in February.

## Result Contract

The method allocates a boolean `Series` with exactly the receiver's rank and
shape. A scalar stays rank zero, higher-rank geometry is retained, and an empty
Timeline produces an empty result. `NaT` becomes `false`. The receiver and its
nanosecond precision are untouched.

## Example 1: Calendar-Year Closings

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_year_end());
```

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[true, false, false, false, false, true, false, false, false]
```

## Example 2: Higher-Rank Result

```cpp
const auto dates = pluss::timeline::make({
    {"2024-01-01T23:59:59.999999999Z", "2024-01-02T00:00:00Z",
     "2024-02-29T00:00:00Z"},
    {"2024-03-31T12:00:00Z", "2024-04-01T00:00:00Z",
     "2024-12-31T00:00:00Z"},
});
pluss::series::print(std::cout, dates.is_year_end());
```

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡ false false false ⎤
⎣ false false  true ⎦
```

## Example 3: Timezone Can Reveal The Previous Year's End

```cpp
const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
const auto local = utc.tz_convert("America/New_York");
std::cout << std::boolalpha
          << "UTC=" << utc.is_year_end().item<bool>() << '\n'
          << "New York=" << local.is_year_end().item<bool>() << '\n';
```

```text
UTC=false
New York=true
```

## Example 4: Empty Input Has No Boundary Members

```cpp
const auto scalar = pluss::timeline::make("2024-12-31T00:00:00Z");
const auto empty = pluss::timeline::full({0}, scalar);
pluss::series::print(std::cout, empty.is_year_end());
```

```text
Series(shape=[0], strides=[1], dtype=bool, platform=cpu, numel=0, contiguous=true)
[]
```

## Example 5: Year Ends Are Quarter Ends

```cpp
const auto ends = dates.is_year_end();
std::cout << std::boolalpha
          << ends.logical_and(dates.is_quarter_end()).equal(ends) << '\n';
```

```text
true
```

## Gotchas And Failure Modes

- This predicate does not calculate an extremum; use `max()` for that job.
- Fiscal and working-day year ends require separate explicit policies.
- `false` includes both valid non-boundary dates and `NaT`.
- Convert to the intended reporting timezone before constructing the mask.

## See Also

- [`is_year_start`](is_year_start.md)
- [`is_quarter_end`](is_quarter_end.md)
- [`max`](max.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_weekend_scalar.html"><span class="label">Previous</span><span class="title">is_weekend_scalar</span></a>
  <a class="next-link" href="is_year_start.html"><span class="label">Next</span><span class="title">is_year_start</span></a>
</div>
<!-- pluss-hand-authored -->
