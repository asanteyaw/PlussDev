<!-- pluss-hand-authored -->

# Series::bitwise_xor

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::bitwise_xor(const Series& rhs) const -> Series
Series& Series::bitwise_xor_(const Series& rhs) const
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

## In-Place Form

`bitwise_xor_()` is the in-place sibling of [`bitwise_xor()`](bitwise_xor.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `bitwise_xor_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({6, 12}, pluss::ScalarType::Int64);
auto y = pluss::make_series({3, 10}, pluss::ScalarType::Int64);

x.bitwise_xor_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[5, 6]
```

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
  <a class="prev-link" href="bitwise_right_shift.html">
    <span class="label">Previous</span>
    <span class="title">bitwise_right_shift</span>
  </a>
  <a class="next-link" href="block.html">
    <span class="label">Next</span>
    <span class="title">block</span>
  </a>
</div>
