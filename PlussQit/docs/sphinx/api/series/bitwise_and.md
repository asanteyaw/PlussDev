<!-- pluss-hand-authored -->

# Series::bitwise_and

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::bitwise_and(const Series& rhs) const -> Series
```

## What It Does

`bitwise_and()` combines two integer-like or boolean `Series` values bit by bit.

For each aligned position, the result bit is `1` only when that bit is `1` in
both inputs. That makes `bitwise_and()` the "keep only the overlap" operator of
the bitwise family.

On ordinary integers, that means the operation works on the stored binary
representation, not on numeric magnitude in the arithmetic sense. On booleans,
the same operation behaves like boolean conjunction, but it stays in the
bitwise family and follows bitwise dtype rules.

## Parameters

**rhs** (`const Series&`)

The right-hand operand.

Load-bearing behavior:

- `rhs` must be broadcast-compatible with `self`
- both operands must have integer or boolean dtype
- floating-point operands are rejected instead of being silently reinterpreted

## Returns

`Series`
    A new `Series` whose values are the bitwise overlap of `self` and `rhs`.

The result shape follows normal broadcasting. The result dtype follows the
library's integer/boolean promotion rules for the participating operands.

## Examples

Example 1. Basic integer masking keeps only the bits shared by both operands.

```cpp
auto left = pluss::make_series({1, 6, 12, 10}, pluss::ScalarType::Int32);
auto right = pluss::make_series({3, 10, 5, 12}, pluss::ScalarType::Int32);

pluss::series::print(std::cout, left.bitwise_and(right));
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[1, 2, 4, 8]
```

The lane-by-lane binary picture is:

- `1 & 3` -> `0001 & 0011 = 0001`
- `6 & 10` -> `0110 & 1010 = 0010`
- `12 & 5` -> `1100 & 0101 = 0100`
- `10 & 12` -> `1010 & 1100 = 1000`

Example 2. Floating-point inputs are not accepted.

```cpp
try {
  auto a = pluss::make_series({1.0, 2.0});
  auto b = pluss::make_series({1.0, 0.0});
  std::cout << a.bitwise_and(b);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
bitwise: dtype must be integer/bool
```

## Gotchas And Failure Modes

- This is not arithmetic multiplication and not logical short-circuiting.
- Floating-point dtypes fail fast.
- Broadcasting still applies, so shape mistakes surface as broadcast errors
  before the bitwise kernel runs.

## Summary

Use `bitwise_and()` when the question is "which bits survive in both inputs?"

## See Also

- [`Series::bitwise_or`](bitwise_or.md): keeps bits that appear in either
  operand.
- [`Series::bitwise_xor`](bitwise_xor.md): keeps bits that differ.
- [`Series::logical_and`](logical_and.md): truth-value logic rather than raw
  bit patterns.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bincount.html">
    <span class="label">Previous</span>
    <span class="title">bincount</span>
  </a>
  <a class="next-link" href="bitwise_not.html">
    <span class="label">Next</span>
    <span class="title">bitwise_not</span>
  </a>
</div>
