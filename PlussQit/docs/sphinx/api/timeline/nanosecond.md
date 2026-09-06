# Timeline::nanosecond

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::nanosecond() const
```

Aliases: `ns()`, `nsec()`, `nano()`, and `nanoseconds()`.

## Read The Fractional Second Exactly

`nanosecond()` returns the fractional portion after the whole second, measured
in nanoseconds. Valid values range from `0` through `999'999'999`. Unlike the
coarser calendar fields, its result dtype is `int64`, preserving the full
nine-digit fraction without narrowing.

This is not an epoch count. `123456789` means “0.123456789 seconds within this
timestamp's current second,” regardless of the date. Shape and element order
are preserved, and `NaT` maps to `-1`.

## Example: Nine Digits Of Subsecond Precision

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.nanosecond());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[123456789, 987654321, 9010011, -1]
```

Leading zeroes are numeric place value, not stored display characters. Thus
`.009010011` appears as the integer `9010011`.

## Choosing A Precision Extractor

- Use `nanosecond()` when all nine fractional digits matter.
- Use `microsecond()` when downstream data is intentionally microsecond-based.
- Use `millisecond()` for millisecond buckets.

The coarser methods discard lower digits by integer truncation; they do not
round. Keep the nanosecond value if later work may need the lost precision.

## Gotchas And Failure Modes

- Do not confuse fractional nanoseconds with `epoch_nanos()`.
- `-1` is the missing sentinel; valid fractions are never negative.
- Timezone conversion changes calendar fields but not this fractional-second
  component, because it preserves the instant.

## See Also

- [`microsecond`](microsecond.md)
- [`millisecond`](millisecond.md)
- [`epoch_nanos`](epoch_nanos.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="second.html"><span class="label">Previous</span><span class="title">second</span></a>
  <a class="next-link" href="microsecond.html"><span class="label">Next</span><span class="title">microsecond</span></a>
</div>
<!-- pluss-hand-authored -->
