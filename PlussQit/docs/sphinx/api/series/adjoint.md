<!-- pluss-hand-authored -->

# Series::adjoint

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::adjoint() const
```

See [`mT()`](mT.md) for the non-conjugating matrix transpose view.

## What It Does

`adjoint()` swaps the **last two axes** and conjugates the values, leaving every
leading batch axis in place. On a stack of matrices it computes each matrix's
conjugate transpose independently. For a real dtype conjugation changes
nothing, but the operation still returns a materialized contiguous result.

For a plain 2-D matrix both are simply the transpose, and match
[`t()`](t.md) and `transpose(0, 1)`.

The difference from [`transpose()`](transpose.md) is that you never name the
axes: `adjoint()` always means "the last two", which is exactly what a
batched linear-algebra expression wants.

## Parameters

None.

## Returns

`Series`
    A contiguous series with its last two axes swapped and values conjugated.

## Gradient

`adjoint()` is differentiable. Swapping axes is its own inverse, so the
backward applies the same last-two-axis swap and conjugation to the upstream
gradient.

## Examples

Example 1. On a real matrix, `adjoint()` is the ordinary transpose returned as
a contiguous array.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});

std::cout << m.adjoint();
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 4],
  [2, 5],
  [3, 6]]
```

Example 2. Rank-1 input is rejected because there are no last two axes.

```cpp
try {
  pluss::make_series({1.0, 2.0, 3.0}).adjoint();
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
adjoint: expected a series with at least 2 dimensions, got a 1-D series
```

## Gotchas And Failure Modes

- Rank `>= 2` is required. Use [`t()`](t.md) if you specifically want the
  no-op-on-vectors behavior.
- `adjoint()` materializes a contiguous result; use [`mT()`](mT.md) when a
  non-conjugating zero-copy transpose view is required.
- For a real dtype conjugation has no numerical effect; it matters for complex
  data.
- To swap two axes that are **not** the last two, use
  [`transpose()`](transpose.md) or [`swapaxes()`](swapaxes.md).

## Summary

`adjoint()` conjugates values and swaps the last two axes of a series:

- batch axes are preserved; each trailing matrix is transposed independently
- it returns a contiguous result
- it requires rank `>= 2` and is differentiable

## See Also

- [`Series::t`](t.md): the 2-D-only transpose that is a no-op on other ranks.
- [`Series::transpose`](transpose.md): swap any two named axes.
- [`Series::swapaxes`](swapaxes.md): swap any two axes of any rank.
- [`Series::mT`](mT.md): non-conjugating last-two-axis transpose view.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="addr.html">
    <span class="label">Previous</span>
    <span class="title">addr</span>
  </a>
  <a class="next-link" href="all.html">
    <span class="label">Next</span>
    <span class="title">all</span>
  </a>
</div>
