<!-- pluss-hand-authored -->

# Series::bitwise_xor

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::bitwise_xor(const Series& rhs) const -> Series
```

## What It Does

`bitwise_xor()` keeps the bits that differ between the two operands.

At each bit position:

- equal bits produce `0`
- different bits produce `1`

That makes exclusive-or useful for toggles, change masks, parity-style checks,
and other "same vs different" bit-pattern work.

## Parameters

**rhs** (`const Series&`)

The right-hand operand. It must be broadcast-compatible with `self` and must
use an integer or boolean dtype.

## Returns

`Series`
    A new `Series` containing the differing bits of the two operands.

## Examples

Example 1. Exclusive-or keeps the differing bit positions.

```cpp
auto left = pluss::make_series({1, 3, 7, 15}, pluss::ScalarType::UInt8);
auto right = pluss::make_series({1, 1, 2, 4}, pluss::ScalarType::UInt8);

pluss::series::print(std::cout, left.bitwise_xor(right));
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=uint8, platform=cpu, numel=4, contiguous=true)
data=[0, 2, 5, 11]
```

Example 2. Floating-point operands are rejected here too.

```cpp
try {
  auto a = pluss::make_series({1.0, 2.0});
  auto b = pluss::make_series({1.0, 0.0});
  std::cout << a.bitwise_xor(b);
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

- `bitwise_xor()` is not exponentiation and not arithmetic subtraction.
- Equal inputs yield zero bits at those positions.
- Floating-point dtypes are not accepted.

## Summary

Use `bitwise_xor()` when the useful information is exactly where the bit
patterns differ.

## See Also

- [`Series::bitwise_and`](bitwise_and.md): overlap only.
- [`Series::bitwise_or`](bitwise_or.md): union of set bits.
- [`Series::logical_xor`](logical_xor.md): truth-value exclusivity rather than
  raw bit-pattern difference.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bitwise_or.html">
    <span class="label">Previous</span>
    <span class="title">bitwise_or</span>
  </a>
  <a class="next-link" href="block.html">
    <span class="label">Next</span>
    <span class="title">block</span>
  </a>
</div>
