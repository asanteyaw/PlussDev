<!-- pluss-hand-authored -->

# Timeline::epoch_micros

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::epoch_micros() const
```

## Project Instants Onto Microsecond Ticks

`epoch_micros()` returns a shape-preserving `Int64` Series whose values count
whole microseconds from `1970-01-01T00:00:00Z`. Conversion uses floor division.
Consequently every instant in `[-1000ns, -1ns]` belongs to microsecond `-1`;
it is not truncated into microsecond `0`.

```cpp
const auto times = pluss::timeline::from_epoch(
    {{-1LL, 0LL}, {1'999'999'999LL, pluss::Timeline::kNaT}});
std::cout << pluss::series::print(times.epoch_micros()) << '\n';
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡       -1                    0 ⎤
⎣  1999999 -9223372036854775808 ⎦
```

Sub-microsecond precision is discarded, so this projection is reversible only
when every valid nanosecond tick is divisible by `1,000`. `NaT` remains
`Timeline::kNaT`; timezone metadata is not carried by the returned Series.

## See Also

- [`Timeline::epoch_nanos`](epoch_nanos.md)
- [`Timeline::epoch_millis`](epoch_millis.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch_nanos.html"><span class="label">Previous</span><span class="title">epoch_nanos</span></a>
  <a class="next-link" href="epoch_millis.html"><span class="label">Next</span><span class="title">epoch_millis</span></a>
</div>
