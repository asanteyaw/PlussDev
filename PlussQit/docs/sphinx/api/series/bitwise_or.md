<!-- pluss-hand-authored -->

# Series::bitwise_or

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::bitwise_or(const Series& rhs) const -> Series
```

## What It Does

`bitwise_or()` combines two operands bit by bit and keeps any bit that is set
in either operand.

If `bitwise_and()` asks "which bits survive in both?", `bitwise_or()` asks
"which bits appear anywhere across the pair?"

## Parameters

**rhs** (`const Series&`)

The right-hand operand. It must broadcast with `self` and use an integer or
boolean dtype.

## Returns

`Series`
    A new `Series` containing the bitwise union of the two operands.

## Examples

Example 1. Integer bitwise union keeps every `1` bit that appears on either
side.

```cpp
auto left = pluss::make_series({1, 6, 12, 10}, pluss::ScalarType::Int32);
auto right = pluss::make_series({3, 10, 5, 12}, pluss::ScalarType::Int32);

std::cout << left.bitwise_or(right);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[3, 14, 13, 14]
```

Example 2. The same dtype restriction applies to the whole bitwise family.

```cpp
try {
  auto a = pluss::make_series({1.0, 2.0});
  auto b = pluss::make_series({1.0, 0.0});
  std::cout << a.bitwise_or(b);
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

- This is not arithmetic addition.
- Floating-point operands fail fast.
- Bitwise methods operate on stored bits, so the result is best read as a bit
  pattern first and a numeric magnitude second.

## Summary

Use `bitwise_or()` when you need the bitwise union of two inputs.

## See Also

- [`Series::bitwise_and`](bitwise_and.md): keeps only overlapping bits.
- [`Series::bitwise_xor`](bitwise_xor.md): keeps differing bits only.
- [`Series::logical_or`](logical_or.md): truth-value union rather than raw
  bit union.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bitwise_not.html">
    <span class="label">Previous</span>
    <span class="title">bitwise_not</span>
  </a>
  <a class="next-link" href="bitwise_xor.html">
    <span class="label">Next</span>
    <span class="title">bitwise_xor</span>
  </a>
</div>
