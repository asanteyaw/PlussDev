<!-- pluss-hand-authored -->

# Series::fill_diagonal

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::fill_diagonal_(double value) const
Series& Series::fill_diagonal_(double value, bool wrap) const
```

## What It Does

`fill_diagonal_()` overwrites the diagonal of a 2D `self` with `value`, in place, leaving every off-diagonal element untouched. There is no out-of-place counterpart on this surface — it exists only as a mutator.

With `wrap=true` on a tall (`rows > cols`) matrix, the diagonal pattern wraps around and continues past the first `cols` rows instead of stopping there.

Despite being declared `const` at the call site, `fill_diagonal_()` genuinely mutates the series it's called on.

## Parameters

**value** (`double`) — the scalar written into each diagonal position.

**wrap** (`bool`, default `false`) — whether the diagonal pattern wraps past the first `cols` rows on a tall matrix.

## Returns

`Series&` — a reference to `self`, after its diagonal has been overwritten with `value`.

## Examples

Example 1. The diagonal of a `2x2` matrix is zeroed; off-diagonal values are untouched.

```cpp
auto x = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
x.fill_diagonal_(0.0);
std::cout << x;
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, 2],
  [3, 0]]
```

## Gotchas And Failure Modes

- Requires a 2D `self` — this is not a batched or N-D operation.
- `wrap` only matters for a non-square, tall matrix.

## Summary

Use `fill_diagonal_()` when you need to set (or reset, e.g. to zero) exactly the diagonal of a matrix without touching anything else.

## See Also

- [`Series::fill`](fill.md): fills every element, not just the diagonal.
- [`Series::diagonal`](diagonal.md): reads the diagonal instead of writing it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fill.html">
    <span class="label">Previous</span>
    <span class="title">fill</span>
  </a>
  <a class="next-link" href="flatten.html">
    <span class="label">Next</span>
    <span class="title">flatten</span>
  </a>
</div>
