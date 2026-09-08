# Timeline::minute

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::minute() const
```

## Read The Minute Within The Hour

`minute()` extracts a zero-based clock component in the range `0` through
`59`. It returns an `int32` Series with the same shape and element ordering as
the Timeline. Absent instants map to `-1`.

The value is only the minute component. For example, `10:07` and `18:07` both
produce `7`; combine it with `hour()` when constructing a time-of-day key.

## Example

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.minute());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[59, 0, 7, -1]
```

## Working With Minute Buckets

`minute()` reports the original field; it does not round. To place timestamps
on five-minute or fifteen-minute boundaries while keeping them as times, use
the rounding methods. Extracting first discards the date, hour, seconds, and
subsecond precision.

## Gotchas And Failure Modes

- Minutes are zero-based clock fields, unlike one-based months and days.
- `minute()` is the sole public spelling; `min()` remains the Timeline
  reduction.
- A `-1` result denotes `NaT` and must not be placed into an ordinary bucket.

## See Also

- [`hour`](hour.md)
- [`second`](second.md)
- [`floor`](floor.md)
- [`round`](round.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html"><span class="label">Previous</span><span class="title">hour</span></a>
  <a class="next-link" href="minute_scalar.html"><span class="label">Next</span><span class="title">minute_scalar</span></a>
</div>
<!-- pluss-hand-authored -->
