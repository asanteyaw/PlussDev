# Timeline::second

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::second() const
```

`second()` is the sole public spelling.

## Read The Whole-Second Component

`second()` extracts the whole-second field in the range `0` through `59`. It
does not include the fractional portion: a timestamp ending in
`08.009010011` returns `8`, while `nanosecond()` returns the fraction.

The output is a shape-preserving `int32` Series. `NaT` produces `-1`.

## Example: Whole And Fractional Parts Stay Separate

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.second());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[59, 0, 8, -1]
```

To reconstruct the within-minute numeric offset at nanosecond precision, pair
the field with `nanosecond()`: conceptually
`second * 1'000'000'000 + nanosecond`. Guard `NaT` first, because both absent
sentinels are `-1` and are not valid temporal components.

## Gotchas And Failure Modes

- `second()` truncates nothing itself; it selects the already stored whole
  second and leaves the subsecond field to the precision extractors.
- It is not epoch seconds. Use an epoch export when absolute numeric time is
  required.
- Equal second fields do not imply equal timestamps.

## See Also

- [`nanosecond`](nanosecond.md)
- [`epoch_nanos`](epoch_nanos.md)
- [`ticks`](ticks.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minute.html"><span class="label">Previous</span><span class="title">minute</span></a>
  <a class="next-link" href="second_scalar.html"><span class="label">Next</span><span class="title">second_scalar</span></a>
</div>
<!-- pluss-hand-authored -->
