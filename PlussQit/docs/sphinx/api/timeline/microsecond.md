# Timeline::microsecond

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::microsecond() const
```

`microsecond()` is the sole public spelling.

## Read The Fraction At Microsecond Resolution

`microsecond()` returns the fractional second measured in whole microseconds.
Its valid range is `0` through `999'999`, its dtype is `int32`, and its shape
matches the Timeline. Absent instants produce `-1`.

The conversion is truncation, not rounding:

```text
microsecond = nanosecond / 1,000
```

Consequently, nanoseconds below one microsecond are deliberately discarded.

## Example: Observe The Truncation

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.microsecond());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[123456, 987654, 9010, -1]
```

The first fraction loses its trailing `789` nanoseconds; the third loses `11`.
That is expected precision reduction, not floating-point error.

## When To Use It

Use this extractor when an interface, grouping key, or storage format is
defined in whole microseconds. If you only want timestamps displayed with six
digits, prefer formatting: extraction converts the fraction to ordinary
numeric data and discards the temporal type.

## Gotchas And Failure Modes

- No rounding occurs at the half-microsecond boundary.
- The value is within the current second, not since the epoch.
- Once lower nanosecond digits are discarded, this result cannot reconstruct
  the original timestamp exactly.

## See Also

- [`nanosecond`](nanosecond.md)
- [`millisecond`](millisecond.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nanosecond.html"><span class="label">Previous</span><span class="title">nanosecond</span></a>
  <a class="next-link" href="millisecond.html"><span class="label">Next</span><span class="title">millisecond</span></a>
</div>
<!-- pluss-hand-authored -->
