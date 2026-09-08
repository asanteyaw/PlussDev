<!-- pluss-hand-authored -->

# Timeline::from_epoch_micros

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
static Timeline Timeline::from_epoch_micros(
    const Series& epoch_micros,
    std::string_view tz_name = "UTC")
```

## Construct From Exact Microsecond Ticks

`from_epoch_micros()` interprets every input integer as a signed count of
microseconds from `1970-01-01T00:00:00Z`, multiplies it by `1,000`, and stores
the resulting nanosecond tick. Rank and shape are preserved.

`tz_name` supplies calendar presentation metadata; it does not shift the input
ticks. Thus `0` always represents the Unix epoch instant, although its rendered
local clock depends on the chosen zone.

```cpp
const auto micros = pluss::make_series({-1LL, 0LL, 1'999'999LL});
const auto times = pluss::Timeline::from_epoch_micros(micros);
std::cout << pluss::series::print(times.epoch_nanos()) << '\n';
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[-1000, 0, 1999999000]
```

`Timeline::kNaT` remains `NaT` instead of being scaled. Values whose product
cannot fit in a nanosecond tick throw; overflow never wraps into another
instant. Non-integer Series values are converted to `Int64` before scaling, so
use an integer Series when fractional handling must be explicit.

## See Also

- [`Timeline::epoch_micros`](epoch_micros.md)
- [`Timeline::from_epoch_millis`](from_epoch_millis.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch_days.html"><span class="label">Previous</span><span class="title">epoch_days</span></a>
  <a class="next-link" href="from_epoch_millis.html"><span class="label">Next</span><span class="title">from_epoch_millis</span></a>
</div>
