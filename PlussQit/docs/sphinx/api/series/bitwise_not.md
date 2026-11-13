<!-- pluss-hand-authored -->

# Series::bitwise_not

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::bitwise_not() const
Series& Series::bitwise_not_() const
```

## What It Does

`bitwise_not()` flips every bit of every element.

For an integer lane, each `0` bit becomes `1` and each `1` bit becomes `0`.
That is why signed results can look surprising at first glance: the operation
acts on the stored bit pattern, so two's-complement negative values are the
natural consequence.

## Returns

`Series`
    A new `Series` with every bit inverted.

The shape is unchanged. The dtype stays in the same integer/boolean family.

## In-Place Form

`bitwise_not_()` is the in-place sibling of [`bitwise_not()`](bitwise_not.md): it computes the same
result and writes it back into `self` directly — mutating it (and any other
handle sharing the same storage) instead of allocating a new `Series` — then
returns a reference to `self`.

Despite being declared `const` at the call site — a quirk of how this method's wrapper is generated, not a promise that `self` is left unchanged — `bitwise_not_()` genuinely mutates the series it's called on.

```cpp
auto x = pluss::make_series({0, -1, 5}, pluss::ScalarType::Int64);
x.bitwise_not_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[-1, 0, -6]
```

## Examples

Example 1. Integer bitwise inversion is a raw bit flip, not an arithmetic
"negate plus one" convenience wrapper.

```cpp
auto bits = pluss::make_series({1, 6, 12, 10}, pluss::ScalarType::Int32);

std::cout << pluss::series::print(bits.bitwise_not());
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[-2, -7, -13, -11]
```

Why those values appear:

- `~1 = -2`
- `~6 = -7`
- `~12 = -13`
- `~10 = -11`

That is standard two's-complement behavior after every bit is inverted.

## Gotchas And Failure Modes

- Expect raw bit inversion, not arithmetic negation.
- Signed outputs often become negative even when the inputs are positive.
- As with the rest of the bitwise family, floating-point dtypes are not valid.

## Summary

Use `bitwise_not()` when the intent is to invert bit patterns directly.

## See Also

- [`Series::bitwise_and`](bitwise_and.md): keep overlapping bits.
- [`Series::bitwise_or`](bitwise_or.md): combine bits from either side.
- [`Series::logical_not`](logical_not.md): truth-value inversion rather than
  raw bit inversion.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bitwise_left_shift.html">
    <span class="label">Previous</span>
    <span class="title">bitwise_left_shift</span>
  </a>
  <a class="next-link" href="bitwise_or.html">
    <span class="label">Next</span>
    <span class="title">bitwise_or</span>
  </a>
</div>
