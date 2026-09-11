# Timeline::is_weekend

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_weekend() const
```

## Saturday-Or-Sunday Classification

`is_weekend()` marks instants whose local civil date is Saturday or Sunday.
It is a fixed two-day rule, suitable when those two weekdays are exactly what
the caller means. It does not infer regional workweeks or consult holidays.

## Result Contract

The method returns a new boolean `Series` and preserves the Timeline's full
shape and ordering. `NaT` maps to `false`; scalar input remains rank zero and
empty input remains empty. The receiver is read-only. Classification follows
the current timezone because weekday is a local-calendar property.

## Example 1: Full Week And `NaT`

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_weekend());
```

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, false, false, true, true, false]
```

## Example 2: Matrix Layout Is Retained

```cpp
const auto dates = pluss::timeline::make({
    {"2024-03-04T12:00:00Z", "2024-03-09T12:00:00Z"},
    {"", "2024-03-10T12:00:00Z"},
});
pluss::series::print(std::cout, dates.is_weekend());
```

```text
Series(shape=[2,2], strides=[2,1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=
⎡ false true ⎤
⎣ false true ⎦
```

## Example 3: Timezone Boundary

```cpp
const auto utc = pluss::timeline::make("2024-03-09T01:30:00Z");
const auto local = utc.tz_convert("America/New_York");
std::cout << std::boolalpha
          << "UTC=" << utc.is_weekend().item<bool>() << '\n'
          << "New York=" << local.is_weekend().item<bool>() << '\n';
```

```text
UTC=true
New York=false
```

## Example 4: Weekend Is The Union Of Two Named Days

```cpp
const auto named = week.is_saturday().logical_or(week.is_sunday());
std::cout << std::boolalpha << named.equal(week.is_weekend()) << '\n';
```

```text
true
```

## Example 5: Empty Input

```cpp
const auto scalar = pluss::timeline::make("2024-03-09T12:00:00Z");
const auto empty = pluss::timeline::full({0}, scalar);
pluss::series::print(std::cout, empty.is_weekend());
```

```text
Series(shape=[0], strides=[1], dtype=bool, platform=cpu, numel=0, contiguous=true)
[]
```

## Gotchas And Failure Modes

- `NaT` is neither weekend nor business day; use `isvalid()` when absence
  needs a separate state.
- Use `is_weekend_scalar()` for a native `bool` from rank-zero input.
- The operation produces a mask; it does not select or reorder timestamps.
- Choose the reporting timezone before creating the mask.

## See Also

- [`is_weekend_scalar`](is_weekend_scalar.md)
- [`is_saturday`](is_saturday.md)
- [`is_sunday`](is_sunday.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_wednesday.html"><span class="label">Previous</span><span class="title">is_wednesday</span></a>
  <a class="next-link" href="is_weekend_scalar.html"><span class="label">Next</span><span class="title">is_weekend_scalar</span></a>
</div>
<!-- pluss-hand-authored -->
