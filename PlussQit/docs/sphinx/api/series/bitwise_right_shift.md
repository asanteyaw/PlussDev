<!-- pluss-hand-authored -->

# Series::bitwise_right_shift

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::bitwise_right_shift(const Series& rhs) const -> Series
Series& Series::bitwise_right_shift_(const Series& rhs) const
```

## What It Does

Elementwise integer right shift: `self >> rhs`, bit for bit.

`bitwise_right_shift_()` is the in-place sibling: it writes the shifted values back into `self` directly — mutating it (and any other handle sharing the same storage) instead of allocating a new `Series` — and returns a reference to `self`. Despite being declared `const` at the call site, it genuinely mutates `self`.

## Parameters

**rhs** (`const Series&`) — the shift amounts, broadcastable against `self`. Both operands must be integer-dtype.

## Returns

`Series` (out-of-place) or `Series&` (in-place) — the shifted values, same integer dtype as `self`.

## Examples

Example 1. Shifting each element right by a per-position amount.

```cpp
auto x = pluss::make_series({16, 8}, pluss::ScalarType::Int64);
auto y = pluss::make_series({2, 1}, pluss::ScalarType::Int64);

x.bitwise_right_shift_(y);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[4, 4]
```

## Gotchas And Failure Modes

- Both `self` and `rhs` must already be integer dtype.

## Summary

Use `bitwise_right_shift()`/`bitwise_right_shift_()` for elementwise `>>` on integer series.

## See Also

- [`Series::bitwise_left_shift`](bitwise_left_shift.md): the inverse direction.
- [`Series::bitwise_and`](bitwise_and.md), [`Series::bitwise_or`](bitwise_or.md): other bitwise ops.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bitwise_or.html">
    <span class="label">Previous</span>
    <span class="title">bitwise_or</span>
  </a>
  <a class="next-link" href="bitwise_xor.html">
    <span class="label">Next</span>
    <span class="title">bitwise_xor</span>
  </a>
</div>
