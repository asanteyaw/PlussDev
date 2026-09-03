<!-- pluss-hand-authored -->

# Series::diagflat

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::diagflat() const -> Series
Series::diagflat(std::int64_t diagonal) const -> Series
```

## What It Does

`diagflat()` flattens the input and writes those flattened values onto a
diagonal of a new square matrix.

On the current verified root surface, that behavior is the same practical
contract as [`diag_embed()`](diag_embed.md): flatten first, embed second, fill
everything else with zero.

So the public difference between the two names is mainly one of intent:

- `diagflat()` advertises the flattening step right in the name
- `diag_embed()` emphasizes the final diagonal-matrix result

## Parameters

**diagonal** (`std::int64_t`)

The diagonal offset.

- `0` means the main diagonal
- positive values write above the main diagonal
- negative values write below the main diagonal

If the flattened input has `n` elements, the output side length is
`n + abs(diagonal)`.

## Returns

`Series`
    A square 2D `Series` containing the flattened input along one diagonal.

## Gradient

`diagflat()` follows the same verified reverse-mode behavior as
[`diag_embed()`](diag_embed.md): the forward pass flattens and writes values
onto one diagonal, and the reverse pass reads sensitivity back from that same
diagonal.

## Examples

Example 1. A matrix input is flattened before embedding.

```cpp
std::cout << pluss::make_series({
    {1, 2},
    {3, 4},
}, pluss::ScalarType::Int64).diagflat();
```

### Output

```text
pluss::Series(shape=[4,4], strides=[4,1], dtype=int64, platform=cpu, numel=16, contiguous=true)
data=[[1, 0, 0, 0],
  [0, 2, 0, 0],
  [0, 0, 3, 0],
  [0, 0, 0, 4]]
```

Example 2. Negative offsets move the flattened values below the main diagonal.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({5, 6}, pluss::ScalarType::Int64).diagflat(-1));
```

### Output

```text
Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=
⎡ 0 0 0 ⎤
⎢ 5 0 0 ⎥
⎣ 0 6 0 ⎦
```

## Gotchas And Failure Modes

- `diagflat()` always flattens first on the current root surface.
- Offsets change output size as well as diagonal position.
- All non-diagonal entries are zero-filled.
- Reverse-mode follows the same diagonal readback rule as
  [`diag_embed()`](diag_embed.md).

## Summary

Use `diagflat()` when you want the flattening step to be part of the public
contract of the call. If you need extraction instead of embedding, `diagflat()`
is the wrong family entirely.

## See Also

- [`Series::diag_embed`](diag_embed.md): same current value behavior under the
  explicit embedding name.
- [`Series::diag`](diag.md): dual-mode extract-or-embed surface.
- [`Series::diagonal`](diagonal.md): extraction-only diagonal surface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diag_embed.html">
    <span class="label">Previous</span>
    <span class="title">diag_embed</span>
  </a>
  <a class="next-link" href="diagonal.html">
    <span class="label">Next</span>
    <span class="title">diagonal</span>
  </a>
</div>
