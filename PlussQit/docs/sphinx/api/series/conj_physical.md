<!-- pluss-hand-authored -->

# Series::conj_physical

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::conj_physical() const
Series& Series::conj_physical_() const
```

## What It Does

Returns the elementwise complex conjugate, **materialized** into real storage rather than a lazy conjugated view. For a real (non-complex) dtype this is a no-op — there is nothing to conjugate, so the values pass through unchanged.

`conj_physical_()` is the in-place sibling: it writes the conjugated values back into `self` directly — mutating it (and any other handle sharing the same storage) instead of allocating a new `Series` — and returns a reference to `self`. Despite being declared `const` at the call site, it genuinely mutates `self`.

## Parameters

None. This is a zero-argument transform on `self`.

## Returns

`Series` (out-of-place) or `Series&` (in-place) — the conjugated values, same dtype and shape as `self`.

## Examples

Example 1. On a real dtype, `conj_physical_()` is a verified no-op.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.conj_physical_();
std::cout << x;
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

## Gotchas And Failure Modes

- Only complex dtypes actually change under this call; real dtypes always pass through unchanged.

## Summary

Use `conj_physical()`/`conj_physical_()` when you need a real, materialized conjugate buffer for complex data rather than a lazily-conjugated view.

## See Also

- [`Series::abs`](abs.md): another elementwise value transform.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="conj.html">
    <span class="label">Previous</span>
    <span class="title">conj</span>
  </a>
  <a class="next-link" href="contiguous.html">
    <span class="label">Next</span>
    <span class="title">contiguous</span>
  </a>
</div>
