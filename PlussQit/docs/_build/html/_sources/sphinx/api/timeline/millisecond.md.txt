# Timeline::millisecond

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::millisecond() const
```

Aliases: `ms()`, `msec()`, `milli()`, and `milliseconds()`.

## Read The Fraction At Millisecond Resolution

`millisecond()` reports the whole milliseconds within each second. Values run
from `0` through `999`; the output is shape-preserving `int32`, with `-1` for
`NaT`.

It uses integer truncation:

```text
millisecond = nanosecond / 1,000,000
```

It does not round the timestamp and does not alter the source Timeline.

## Example

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.millisecond());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[123, 987, 9, -1]
```

The third result is `9`, not `009`: integers preserve value, not textual
padding. Formatting controls how many digits are displayed.

## Extraction Versus Rounding

Use `millisecond()` to obtain a numeric feature or component. Use `floor()`,
`ceil()`, or `round()` when the desired result should remain a Timeline aligned
to a temporal boundary. Those are different operations: extraction discards
the rest of the timestamp; rounding transforms the timestamp as a whole.

## Gotchas And Failure Modes

- The method truncates sub-millisecond digits instead of rounding them.
- The result is a component in `[0, 999]`, not epoch milliseconds.
- `-1` denotes missingness and should be masked before unsigned conversion.

## See Also

- [`microsecond`](microsecond.md)
- [`floor`](floor.md)
- [`round`](round.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="microsecond.html"><span class="label">Previous</span><span class="title">microsecond</span></a>
  <a class="next-link" href="weekday.html"><span class="label">Next</span><span class="title">weekday</span></a>
</div>
<!-- pluss-hand-authored -->
