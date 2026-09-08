<!-- pluss-hand-authored -->

# Timeline::epoch_seconds

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::epoch_seconds() const
```

## Project Instants Onto Whole Seconds

`epoch_seconds()` maps each instant to the whole UTC epoch-second interval that
contains it. The return value is an `Int64` Series with the Timeline's shape.

```cpp
const auto times = pluss::timeline::from_epoch(
    {{-1LL, 0LL}, {1'999'999'999LL, pluss::Timeline::kNaT}});
std::cout << pluss::series::print(times.epoch_seconds()) << '\n';
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡ -1                    0 ⎤
⎣  1 -9223372036854775808 ⎦
```

The first position is one nanosecond before the epoch and therefore lies in
second `-1`. The third is almost two seconds after the epoch and lies in second
`1`. This floor rule is deliberately different from truncating signed integer
division toward zero.

`NaT` remains the sentinel. Fractional seconds and timezone metadata are not
represented in the result.

## See Also

- [`Timeline::epoch_millis`](epoch_millis.md)
- [`Timeline::epoch_days`](epoch_days.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch_millis.html"><span class="label">Previous</span><span class="title">epoch_millis</span></a>
  <a class="next-link" href="epoch_days.html"><span class="label">Next</span><span class="title">epoch_days</span></a>
</div>
