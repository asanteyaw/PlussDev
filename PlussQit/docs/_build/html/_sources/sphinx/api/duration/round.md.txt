<!-- pluss-hand-authored -->

# Duration::round

[Back to Duration API Reference](../../duration_api_reference)

## Signature

```cpp
Duration Duration::round(std::string_view unit) const;
```

## Nearest Fixed-Width Boundary

`round()` maps each lane to the nearest exact multiple of `unit`. Its tie rule
is **half to even**: when a value is exactly halfway between two multiples,
the multiple whose quotient is even wins. This avoids the systematic drift
that repeated half-away-from-zero rounding introduces.

For example, in seconds, `0.5 -> 0`, `1.5 -> 2`, `2.5 -> 2`, and
`-1.5 -> -2`. The rule is symmetric around zero and is evaluated with integer
nanosecond arithmetic, so no floating-point conversion can perturb a tie.

Accepted units are `"ns"`, `"us"`, `"ms"`, `"s"`, `"min"`, `"h"`, `"D"`,
and `"W"`. The result is a new shape-preserving `Duration`; `NaT` remains
`NaT`. A result outside the representable nanosecond range throws instead of
wrapping.

## Examples

Example 1. Half-even ties differ from both floor and ceiling.

```cpp
auto values = pluss::duration::nanos({
    -1'600'000'000LL, -1'500'000'000LL, -500'000'000LL,
     500'000'000LL,  1'500'000'000LL, 2'500'000'000LL,
     1'400'000'000LL, pluss::Duration::kNaT,
});
std::cout << pluss::series::print(values.round("s").total().nanoseconds());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[-2.0000e+09, -2.0000e+09, 0.0000e+00, 0.0000e+00, 2.0000e+09, 2.0000e+09, 1.0000e+09, nan]
```

Example 2. Values around ties show which side owns the boundary.

```cpp
auto matrix = pluss::duration::nanos(
    {499'999'999, 500'000'000, 1'499'999'999, 1'500'000'000}, {2,2});
std::cout << pluss::series::print(matrix.round("s").total().nanoseconds());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 0.0000e+00 0.0000e+00 ⎤
⎣ 1.0000e+09 2.0000e+09 ⎦
```

Example 3. Fine and coarse units use the same rule.

```cpp
auto precise = pluss::duration::nanos(1'234'567'891LL);
auto micros = precise.round("us");
auto minutes = precise.round("min");
```

Example 4. Validation is explicit and stable.

```cpp
pluss::duration::seconds(1).round("second");
// throws: Duration: unsupported unit
```

## See Also

- [`floor()`](floor.md) and [`ceil()`](ceil.md): directed rounding.
- [`total().nanoseconds()`](total_nanoseconds.md): inspect the exact stored result.

