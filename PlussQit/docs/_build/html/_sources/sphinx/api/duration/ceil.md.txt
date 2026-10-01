<!-- pluss-hand-authored -->

# Duration::ceil

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::ceil(std::string_view unit) const;
```

## Advance To A Unit Boundary

`ceil()` replaces every elapsed value with the smallest exact multiple of
`unit` that is not less than that value. Exact multiples remain unchanged.
This is fixed-width elapsed-time rounding: a day is exactly 86,400 seconds and
a week is exactly seven such days. No calendar, timezone, or daylight-saving
rule participates.

The accepted unit spellings are `"ns"`, `"us"`, `"ms"`, `"s"`, `"min"`,
`"h"`, `"D"`, and `"W"`. They are deliberately canonical and case-sensitive;
descriptive alternatives such as `"second"` are rejected rather than guessed.

The operation allocates a new `Duration`, preserves rank and shape, and leaves
`NaT` lanes unchanged. Negative values still obey the mathematical ceiling:
`-1.6 s` and `-1.5 s` both become `-1 s`, moving toward positive infinity.

## Examples

Example 1. Positive and negative values advance independently to seconds.

```cpp
auto values = pluss::duration::nanos({
    -1'600'000'000LL, -1'500'000'000LL, -500'000'000LL,
     500'000'000LL,  1'500'000'000LL, 2'500'000'000LL,
     1'400'000'000LL, pluss::Duration::kNaT,
});
std::cout << pluss::series::print(values.ceil("s").total().nanoseconds());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[-1.0000e+09, -1.0000e+09, 0.0000e+00, 1.0000e+09, 2.0000e+09, 3.0000e+09, 2.0000e+09, nan]
```

Example 2. A matrix keeps its shape; only its lanes change.

```cpp
auto matrix = pluss::duration::nanos(
    {1, 1'000'000'000, 1'000'000'001, pluss::Duration::kNaT}, {2, 2});
std::cout << pluss::duration::print(matrix.ceil("s"));
```

The results are `1 s`, `1 s`, `2 s`, and `NaT`, arranged in the original
`[2,2]` shape. An exact boundary is not advanced a second time.

Example 3. Invalid units and unrepresentable rounded values fail before a
partial result is returned.

```cpp
pluss::duration::seconds(1).ceil("second"); // throws: Duration: unsupported unit

pluss::duration::nanos((std::numeric_limits<std::int64_t>::max)())
    .ceil("D");                              // throws: Duration rounding overflow
```

## See Also

- [`floor()`](floor.md): move to the boundary at or below each value.
- [`round()`](round.md): choose the nearest boundary with half-even ties.

