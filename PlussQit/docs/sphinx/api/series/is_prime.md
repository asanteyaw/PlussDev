<!-- pluss-hand-authored -->

# Series::is_prime

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::is_prime() const -> Series
```

## What It Does

`is_prime()` tests each element for primality and returns a boolean mask with
the same shape as the input.

The method is elementwise, but not arithmetic in the usual continuous sense. It
interprets each element as an integer candidate:

- values less than `2` are not prime
- prime values produce `true`
- composite values produce `false`

This is a shape-preserving classification method, not a set reduction and not a
numeric transform.

## Input Rules

`is_prime()` accepts integral and boolean dtypes, except the current `UInt64`
surface.

Accepted families include:

- `Bool`
- signed integers
- `UInt8`, `UInt16`, `UInt32`

The result dtype is always `bool`.

## Returns

`Series`
    A boolean `Series` with the same shape as `self`.

## Gradient

`is_prime()` is not differentiable.

It is a discrete classification by integer value.

## Examples

Example 1. Values below `2` are always reported as non-prime.

```cpp
auto values = pluss::make_series({-2, -1, 0, 1, 2, 3, 4, 17}, pluss::ScalarType::Int64);

std::cout << pluss::series::print(values.is_prime());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, false, true, true, false, true]
```

Example 2. The method preserves shape instead of flattening.

```cpp
std::cout << pluss::make_series({
    {2, 4, 5},
    {15, 17, 19},
}, pluss::ScalarType::Int64).is_prime();
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[true, false, true],
  [false, true, true]]
```

Example 3. Unsigned integer input is accepted too.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({2, 9, 11, 12}, pluss::ScalarType::UInt32).is_prime());
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, true, false]
```

Example 4. Floating input is rejected.

```cpp
try {
  std::cout << pluss::make_series({2.0, 3.0, 4.0}).is_prime() << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
is_prime: expects integral/bool dtype (UInt64 currently unsupported)
```

## Gotchas And Failure Modes

- `is_prime()` is elementwise and shape-preserving.
- Values less than `2` always yield `false`.
- The result dtype is `bool`, not the input dtype.
- Floating-point input fails.
- `UInt64` is currently outside the supported public contract.

## Summary

Use `is_prime()` when you need a prime/non-prime mask over integer data.

- it preserves the input shape
- it returns `bool`
- values below `2` are never prime
- only integral and boolean inputs are accepted on the current surface

## See Also

- [`Series::prime`](prime.md): map ordinal positions such as `1`, `2`, `3`
  into actual prime values.
- [`Series::factor`](factor.md): extract the least prime factor instead of a
  boolean decision.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid_index.html">
    <span class="label">Previous</span>
    <span class="title">invalid_index</span>
  </a>
  <a class="next-link" href="isfinite.html">
    <span class="label">Next</span>
    <span class="title">isfinite</span>
  </a>
</div>
