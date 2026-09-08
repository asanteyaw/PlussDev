<!-- pluss-hand-authored -->

# Timeline::epoch_days

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::epoch_days() const
```

## Project Instants Onto Fixed UTC Days

`epoch_days()` counts fixed 86,400-second intervals from the UTC Unix epoch and
preserves the Timeline's shape. It is a machine-time projection, not a local
calendar-date extractor. For local calendar work, convert to the intended zone
and use [`date()`](date.md), [`day()`](day.md), or
[`dayofyear()`](dayofyear.md).

```cpp
const auto times = pluss::timeline::from_epoch(
    {{-1LL, 0LL}, {1'999'999'999LL, pluss::Timeline::kNaT}});
std::cout << pluss::series::print(times.epoch_days()) << '\n';
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡ -1                    0 ⎤
⎣  0 -9223372036854775808 ⎦
```

One nanosecond before the epoch belongs to UTC day `-1`; both valid positive
ticks shown are still in UTC day `0`. Floor division supplies that boundary
behavior. `NaT` remains `Timeline::kNaT`.

Local days can be 23 or 25 hours around offset transitions, whereas an epoch
day here is always exactly 86,400 seconds. Do not use this method as a local
date key unless that distinction is intentional.

## See Also

- [`Timeline::epoch_seconds`](epoch_seconds.md)
- [`Timeline::date`](date.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty_like.html"><span class="label">Previous</span><span class="title">empty_like</span></a>
  <a class="next-link" href="from_epoch_micros.html"><span class="label">Next</span><span class="title">from_epoch_micros</span></a>
</div>
