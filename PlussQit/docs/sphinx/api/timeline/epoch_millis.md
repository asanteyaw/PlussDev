<!-- pluss-hand-authored -->

# Timeline::epoch_millis

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::epoch_millis() const
```

## Project Instants Onto Millisecond Ticks

`epoch_millis()` counts whole milliseconds from the UTC Unix epoch in a
shape-preserving `Int64` Series. The operation floors toward negative infinity,
which keeps the interval represented by each integer consistent on both sides
of the epoch.

```cpp
const auto times = pluss::timeline::from_epoch(
    {{-1LL, 0LL}, {1'999'999'999LL, pluss::Timeline::kNaT}});
std::cout << pluss::series::print(times.epoch_millis()) << '\n';
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡    -1                    0 ⎤
⎣  1999 -9223372036854775808 ⎦
```

The result is independent of `tz()`: a timezone changes civil presentation,
not distance from the epoch. Nanosecond precision below a millisecond is lost,
and `NaT` is preserved as `Timeline::kNaT`.

## See Also

- [`Timeline::epoch_micros`](epoch_micros.md)
- [`Timeline::epoch_seconds`](epoch_seconds.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch_micros.html"><span class="label">Previous</span><span class="title">epoch_micros</span></a>
  <a class="next-link" href="epoch_seconds.html"><span class="label">Next</span><span class="title">epoch_seconds</span></a>
</div>
