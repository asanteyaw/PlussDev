<!-- pluss-hand-authored -->

# Series::factor

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::factor() const -> Series
```

## What It Does

`factor()` returns the least prime factor of each element.

For ordinary positive composites, that means the smallest prime that divides
the value:

- `12` becomes `2`
- `49` becomes `7`
- `29` becomes `29` because a prime is its own least prime factor

The method also has explicit special cases that are important to know:

- `0` returns `0`
- `1` returns `1`
- `-1` returns `1`
- negative values are factored by magnitude, so `-15` returns `3`

## Input Rules

`factor()` accepts integral and boolean dtypes, except the current `UInt64`
surface, and returns `int64`.

Boolean input is interpreted numerically:

- `false` becomes `0`
- `true` becomes `1`

## Returns

`Series`
    An `int64` `Series` with the same shape as `self`, where each element is
    the least prime factor according to the method's special-case rules.

## Gradient

`factor()` is not differentiable.

It is a discrete integer-analysis operation.

## Examples

Example 1. Composite, prime, zero, one, and negative inputs all follow the
documented special-case rules.

```cpp
auto values = pluss::make_series({0, 1, 2, 12, 49, -15}, pluss::ScalarType::Int64);

std::cout << pluss::series::print(values.factor());
```

### Output

```text
Series(shape=[6], strides=[1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[0, 1, 2, 2, 7, 3]
```

Example 2. Shape is preserved.

```cpp
std::cout << pluss::make_series({
    {-21, 25},
    {27, 29},
}, pluss::ScalarType::Int64).factor();
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[[3, 5],
  [3, 29]]
```

Example 3. Unsigned input is accepted and still returns `int64`.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({0, 1, 14, 25}, pluss::ScalarType::UInt16).factor());
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 1, 2, 5]
```

Example 4. The most negative signed 64-bit value is rejected because its
magnitude cannot be taken safely.

```cpp
try {
  std::cout << pluss::make_series(
      {std::numeric_limits<std::int64_t>::min()},
      pluss::ScalarType::Int64).factor() << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
factor: value too small to factor safely
```

## Gotchas And Failure Modes

- `factor()` returns the least prime factor, not a full factorization.
- `0`, `1`, and `-1` have explicit special-case outputs.
- Negative inputs are factored by magnitude.
- The result dtype is always `int64`.
- The minimum signed 64-bit value is rejected.

## Summary

Use `factor()` when you need one prime divisor per element, not the full prime
decomposition.

- primes return themselves
- composites return their smallest prime divisor
- `0`, `1`, and `-1` use explicit special-case outputs
- negative values are analyzed by magnitude

## See Also

- [`Series::is_prime`](is_prime.md): boolean primality test.
- [`Series::prime`](prime.md): ordinal-to-prime lookup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extract_put_.html">
    <span class="label">Previous</span>
    <span class="title">extract_put_</span>
  </a>
  <a class="next-link" href="factorial.html">
    <span class="label">Next</span>
    <span class="title">factorial</span>
  </a>
</div>
