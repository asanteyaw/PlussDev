<!-- pluss-hand-authored -->

# Timeline::date

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::date() const
```

## Retain Each Local Calendar Date

`date()` replaces the clock fields of every represented instant with local
midnight while preserving shape and timezone. “Local” is essential: the
Timeline first interprets each instant in `tz()`, finds that civil date, and
then resolves midnight in the same zone.

This differs from dividing epoch ticks into 86,400-second blocks. Use
[`epoch_days()`](epoch_days.md) for fixed UTC intervals; use `date()` when the
calendar date visible in the Timeline's zone is the identity that matters.

```cpp
const auto times = pluss::timeline::make({
    "2024-11-01T09:00:00Z",
    "2024-11-01T23:59:59Z",
    "",
});
std::cout << pluss::timeline::print(times.date()) << '\n';
```

### Output

```text
Timeline(shape=[3], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:00:00, NaT ]
```

`NaT` remains `NaT`. The method is out of place and retains nanosecond storage,
although every valid output is aligned to a local day boundary.

## See Also

- [`Timeline::time`](time.md)
- [`Timeline::floor`](floor.md)
- [`Timeline::epoch_days`](epoch_days.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html"><span class="label">Previous</span><span class="title">round</span></a>
  <a class="next-link" href="time.html"><span class="label">Next</span><span class="title">time</span></a>
</div>
