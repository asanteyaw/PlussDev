<!-- pluss-hand-authored -->

# Series::remainder

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::remainder(const Series& other) const -> Series
Series& Series::remainder_(const Series& other)
```

## What It Does

`remainder()` is the elementwise modulo of `a / b` that **keeps the sign of the
divisor** `b`:

```
remainder(a, b) = a - floor(a / b) * b
```

`floor` rounds toward negative infinity, so the result always lands in the
half-open interval between `0` and `b` (`[0, b)` when `b > 0`, `(b, 0]` when
`b < 0`):

- `remainder(-5, 3) = 1` — in `[0, 3)`
- `remainder(5, -3) = -1` — in `(-3, 0]`

This is the "wrap into a range" operation: mapping an arbitrary index onto
`[0, n)`, folding an angle into `[0, 2pi)`, or bucketing a value by a positive
period. When you instead want the leftover to point the same way as the
dividend, use [`fmod()`](fmod.md).

## Parameters

**other** (`const Series&`)

The divisor `b`, which also sets the result's sign and range. Broadcasts
against the receiver.

## Returns

`Series` or `Series&`
    The floored-division remainder, with the sign of `b`. Integer inputs stay
    integer. Division by `0` yields `NaN` (floating point). `remainder_()`
    writes the result back into the receiver.

## Gradient

`remainder()` is differentiable. `remainder(a, b) = a - floor(a / b) * b`, and
`floor` is locally constant, so between the jumps:

```
d/da remainder(a, b) = 1
d/db remainder(a, b) = -floor(a / b)
```

The backward passes the upstream sensitivity straight through to the dividend
and routes `-floor(a / b)` to the divisor. Gradient does not flow through the
discrete step where `floor(a / b)` changes.

## Examples

Example 1. The result follows the sign of the divisor.

```cpp
auto a = pluss::make_series({-5.0, 5.0, -5.0, 5.5});
auto b = pluss::make_series({3.0, -3.0, -3.0, 2.0});

pluss::series::print(std::cout, a.remainder(b));
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, -1.0000, -2.0000, 1.5000]
```

Every result has the same sign as its `b` (or is zero), unlike
[`fmod()`](fmod.md).

Example 2. Integer inputs stay integer. This is the "wrap onto `[0, n)`" case.

```cpp
auto a = pluss::make_series({7, -7, 7}, pluss::ScalarType::Int64);
auto b = pluss::make_series({3, 3, -3}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, a.remainder(b));
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, -2]
```

`-7` wrapped onto `[0, 3)` is `2`, where [`fmod()`](fmod.md) would give `-1`.

Example 3. Side by side with `fmod()`.

```cpp
auto a = pluss::make_series({-5.0, 5.0});
auto b = pluss::make_series({3.0, -3.0});

std::cout << "fmod      = " << a.fmod(b) << "\n";
std::cout << "remainder = " << a.remainder(b) << "\n";
```

### Output

```text
fmod      = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-2, 2]
remainder = pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, -1]
```

Example 4. Division by zero is `NaN`, not an error.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({5.0}).remainder(pluss::make_series({0.0})));
```

### Output

```text
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[nan]
```

Example 5. The backward passes gradient straight through to the dividend and
routes `-floor(a / b)` to the divisor.

```cpp
auto a = pluss::make_series({-5.0, 5.5, 7.0});
auto b = pluss::make_series({3.0, 2.0, 4.0});
a.tracks_slope_(true);
b.tracks_slope_(true);

a.remainder(b).sum().reverse();

std::cout << "da=" << a.sensitivity();
std::cout << "db=" << b.sensitivity();
```

### Output

```text
da=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
db=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, -2, -1]
```

Each `db` entry is `-floor(a / b)`: `-floor(-5/3) = 2`, `-floor(5.5/2) = -2`,
`-floor(7/4) = -1`.

## Gotchas And Failure Modes

- `remainder()` follows the sign of `b`; [`fmod()`](fmod.md) follows the sign of
  `a`. Pick by which range you want the result in.
- Division by `0` gives `NaN`; it does not throw.
- The gradient through the dividend is `1`; no gradient flows through the
  discrete step where `floor(a / b)` changes.
- `NaN` in either input propagates.

## Summary

`remainder(a, b)` is the floored-division modulo, signed like `b`:

- `a - floor(a / b) * b`; result lies between `0` and `b`
- broadcasts like `add()`; preserves integer dtype
- `NaN` on divide-by-zero
- differentiable: `d/da = 1`, `d/db = -floor(a / b)`, no flow through the jumps

## See Also

- [`Series::fmod`](fmod.md): the remainder signed like the dividend instead.
- [`Series::floor`](floor.md): the rounding rule `remainder()` uses internally.
- [`Series::div`](div.md): the quotient this is the leftover of.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="relu.html">
    <span class="label">Previous</span>
    <span class="title">relu</span>
  </a>
  <a class="next-link" href="renorm.html">
    <span class="label">Next</span>
    <span class="title">renorm</span>
  </a>
</div>
