<!-- pluss-hand-authored -->

# Timeline::from_epoch_seconds

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
static Timeline Timeline::from_epoch_seconds(
    const Series& epoch_seconds,
    std::string_view tz_name = "UTC")
```

## Construct Whole-Second Instants

`from_epoch_seconds()` interprets each input as an absolute UTC epoch-second
count and stores the corresponding nanoseconds. It is appropriate for system
interfaces that expose integral seconds and for lossless round trips from
[`epoch_seconds()`](epoch_seconds.md) when every original instant was aligned
to a second boundary.

```cpp
const auto seconds = pluss::make_series({{-1LL, 0LL}, {1LL, pluss::Timeline::kNaT}});
const auto times = pluss::Timeline::from_epoch_seconds(
    seconds, "America/New_York");

std::cout << pluss::series::print(times.epoch_nanos()) << '\n'
          << "shape=[" << times.sizes()[0] << ',' << times.sizes()[1]
          << "], tz=" << times.tz() << '\n';
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡          -1000000000                    0 ⎤
⎣           1000000000 -9223372036854775808 ⎦
shape=[2,2], tz=America/New_York
```

The numeric ticks are unchanged by the presentation zone. Shape is preserved,
`NaT` remains `NaT`, and multiplication overflow throws. Converting a Timeline
to whole seconds and back discards its original subsecond remainder.

## See Also

- [`Timeline::epoch_seconds`](epoch_seconds.md)
- [`Timeline::from_ymd`](from_ymd.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_epoch_millis.html"><span class="label">Previous</span><span class="title">from_epoch_millis</span></a>
  <a class="next-link" href="from_ymd.html"><span class="label">Next</span><span class="title">from_ymd</span></a>
</div>
