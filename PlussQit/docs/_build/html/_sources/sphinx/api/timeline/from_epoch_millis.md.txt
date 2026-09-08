<!-- pluss-hand-authored -->

# Timeline::from_epoch_millis

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
static Timeline Timeline::from_epoch_millis(
    const Series& epoch_millis,
    std::string_view tz_name = "UTC")
```

## Construct From Millisecond Ticks

`from_epoch_millis()` maps each signed millisecond count to its exact
nanosecond instant by multiplying by `1,000,000`. The input shape becomes the
Timeline shape; the operation neither flattens nor broadcasts.

```cpp
const auto millis = pluss::make_series({-1LL, 0LL, 1'999LL});
const auto times = pluss::Timeline::from_epoch_millis(millis);
std::cout << pluss::series::print(times.epoch_nanos()) << '\n';
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[-1000000, 0, 1999000000]
```

Negative values retain their sign and exact boundary. `Timeline::kNaT`
propagates as the absent-instant sentinel. An input outside the representable
nanosecond range throws before multiplication, including a value that would
collide with the reserved sentinel.

Timezone metadata changes rendering and field extraction only. It does not
reinterpret these absolute epoch counts as local wall-clock values.

## See Also

- [`Timeline::epoch_millis`](epoch_millis.md)
- [`Timeline::from_epoch_seconds`](from_epoch_seconds.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_epoch_micros.html"><span class="label">Previous</span><span class="title">from_epoch_micros</span></a>
  <a class="next-link" href="from_epoch_seconds.html"><span class="label">Next</span><span class="title">from_epoch_seconds</span></a>
</div>
