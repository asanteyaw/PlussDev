<!-- pluss-hand-authored -->

# Duration::floor

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::floor(std::string_view unit) const;
```

## Retreat To A Unit Boundary

`floor()` replaces each lane with the greatest exact multiple of `unit` that
is not greater than the original elapsed value. It is the operation to use
when a duration must be assigned to a completed fixed-width interval.

The accepted, case-sensitive spellings are `"ns"`, `"us"`, `"ms"`, `"s"`,
`"min"`, `"h"`, `"D"`, and `"W"`. Days and weeks mean fixed elapsed widths,
not calendar periods. The result is newly allocated, has the same shape as the
receiver, and preserves every `NaT` lane.

For negatives, floor moves toward negative infinity. Thus `-0.5 s` floors to
`-1 s`, not zero. This agrees with the normalized component model where a
negative sub-day span occupies the preceding whole day plus nonnegative
sub-day fields.

## Examples

Example 1. Floor a mixed vector to complete seconds.

```cpp
auto values = pluss::duration::nanos({
    -1'600'000'000LL, -1'500'000'000LL, -500'000'000LL,
     500'000'000LL,  1'500'000'000LL, 2'500'000'000LL,
     1'400'000'000LL, pluss::Duration::kNaT,
});
std::cout << pluss::series::print(values.floor("s").total().nanoseconds());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[-2.0000e+09, -2.0000e+09, -1.0000e+09, 0.0000e+00, 1.0000e+09, 2.0000e+09, 1.0000e+09, nan]
```

Example 2. The same value can be bucketed at any canonical width.

```cpp
auto value = pluss::duration::nanos(93'784'005'006'007LL);
std::cout << pluss::duration::print(value.floor("h"));
```

### Output

```text
Duration(shape=[], dim=0, numel=1)
1 days 02:00:00.000000000
```

Using `"ns"` would leave that value unchanged; `"us"` and `"ms"` discard
only finer fractions; `"D"` produces exactly one day.

Example 3. Scalar rank and N-D shape are both preserved.

```cpp
auto matrix = pluss::duration::nanos({1, -1, 1'500'000'000, -1'500'000'000},
                                     {2, 2});
auto result = matrix.floor("s");
// result.sizes() == [2,2]
```

An unsupported spelling throws `Duration: unsupported unit`; no lane is
returned with an inferred unit.

## See Also

- [`ceil()`](ceil.md): move to the boundary at or above each value.
- [`round()`](round.md): select the nearest boundary.

