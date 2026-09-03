<!-- pluss-hand-authored -->

# Series::prime

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::prime() const -> Series
```

## What It Does

`prime()` interprets each element as a 1-based ordinal and returns the
corresponding prime number.

That means:

- `1` maps to `2`
- `2` maps to `3`
- `3` maps to `5`
- `4` maps to `7`

and so on.

This is not the same question as [`is_prime()`](is_prime.md). `is_prime()`
asks whether a value already is prime. `prime()` asks for the nth prime.

## Input Rules

`prime()` accepts the same integral/bool families as the rest of this small
number-theory cluster, but the value rules are stricter:

- every element must be at least `1`
- every element must be at most `200000` on the current direct routine

The output dtype is always `int64`.

Boolean input is therefore interpreted numerically:

- `true` becomes `1`, which maps to prime `2`
- `false` becomes `0`, which is invalid

## Returns

`Series`
    An `int64` `Series` with the same shape as `self`, where each element is
    the requested nth prime.

## Gradient

`prime()` is not differentiable.

It is a discrete ordinal-to-prime lookup.

## Examples

Example 1. Basic ordinal input maps to the corresponding prime values.

```cpp
auto ordinals = pluss::make_series({1, 2, 3, 5}, pluss::ScalarType::Int64);

std::cout << ordinals.prime();
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[2, 3, 5, 11]
```

Example 2. Shape is preserved.

```cpp
pluss::series::print(std::cout, pluss::make_series({
    {1, 4},
    {6, 8},
}, pluss::ScalarType::Int64).prime());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡  2  7 ⎤
⎣ 13 19 ⎦
```

Example 3. Boolean `true` is interpreted as ordinal `1`.

```cpp
std::cout << pluss::make_series({true, true, true, true}, pluss::ScalarType::Bool).prime();
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[2, 2, 2, 2]
```

That is a real contract detail, not a documentation trick: boolean input is
cast to integer meaning before the lookup happens.

Example 4. Zero is invalid because ordinals are 1-based.

```cpp
try {
  std::cout << pluss::make_series({0}, pluss::ScalarType::Int64).prime() << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
prime: expects n >= 1
```

Example 5. Very large ordinals are rejected on the current surface.

```cpp
try {
  std::cout << pluss::make_series({200001}, pluss::ScalarType::Int64).prime() << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
prime: n too large for direct nth-prime routine
```

## Gotchas And Failure Modes

- `prime()` is 1-based, not 0-based.
- Boolean `true` maps to `2`; boolean `false` fails because it becomes `0`.
- The output dtype is always `int64`.
- The current public limit is `n <= 200000`.
- Floating-point input is rejected through the shared number-theory dtype check.

## Summary

Use `prime()` when your input values are ordinal requests for prime numbers.

- `1` means the first prime, not the value `1`
- the result keeps the input shape
- the result dtype is `int64`
- valid inputs must satisfy `1 <= n <= 200000` on the current surface

## See Also

- [`Series::is_prime`](is_prime.md): test whether existing values are prime.
- [`Series::factor`](factor.md): decompose existing values by least prime
  factor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prelu.html">
    <span class="label">Previous</span>
    <span class="title">prelu</span>
  </a>
  <a class="next-link" href="prod.html">
    <span class="label">Next</span>
    <span class="title">prod</span>
  </a>
</div>
