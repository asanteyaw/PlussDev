<!-- pluss-hand-authored -->

# Series::triu_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::triu_() const -> Series&
Series::triu_(std::int64_t diagonal) const -> Series&
```

## What It Does

`triu_()` is the in-place sibling of [`triu()`](triu.md).

It zeroes the disallowed lower-triangular region directly inside the receiver,
then returns that same receiver by reference.

## Overload Map

**`triu_()`**

Equivalent to `triu_(0)`.

**`triu_(std::int64_t diagonal)`**

Mutating upper-triangular masking with an explicit keep boundary.

## Parameters

**diagonal** (`std::int64_t`)

The lowest diagonal that remains non-zero after the mutation.

## Returns

`Series&`
    The mutated receiver.

## Examples

Example 1. `triu_()` mutates the current `Series` and returns it.

```cpp
auto square = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

pluss::series::print(std::cout, square.triu_(-1));
```

### Output

```text
Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=
⎡ 1 2 3 ⎤
⎢ 4 5 6 ⎥
⎣ 0 8 9 ⎦
```

## Gotchas And Failure Modes

- `triu_()` mutates the receiver. Use [`triu()`](triu.md) when you need a copy.
- The zero-argument overload is just the main-diagonal case.
- Because it returns `Series&`, it can be chained into code that expects the
  updated object immediately.

## Summary

Use `triu_()` when the upper-triangular mask should become the current state of
the `Series` rather than a separate derived result.

## See Also

- [`Series::triu`](triu.md): copy-returning upper-triangular masking.
- [`Series::tril_`](tril_.md): in-place lower-triangular companion.
- [`Series::storage_version`](storage_version.md): useful when you need to
  detect later that a mutation occurred.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="triu.html">
    <span class="label">Previous</span>
    <span class="title">triu</span>
  </a>
  <a class="next-link" href="trunc.html">
    <span class="label">Next</span>
    <span class="title">trunc</span>
  </a>
</div>
