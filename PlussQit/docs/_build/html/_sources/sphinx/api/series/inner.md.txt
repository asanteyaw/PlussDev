<!-- pluss-hand-authored -->

# Series::inner

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::inner(const Series& other) const -> Series
```

## What It Does

`inner()` is the **inner product** — the sum of elementwise products of the two
operands after flattening:

```
inner(a, b) = sum_k (flatten(a)[k] * flatten(b)[k])
```

Both operands are flattened to 1-D and must then have the same number of
elements; the result is a scalar `Series` (shape `[]`). It is the contraction
that complements [`outer()`](outer.md): `outer` fans two vectors out to a
matrix, `inner` collapses them to a scalar.

For two genuine 1-D vectors this is the dot product; see
[`SeriesLinearAlgAccessor::dot`](../series-linearalg/dot.md), which enforces the
rank-1 contract instead of flattening.

## Shape Versus Pairing

Only linear element order matters. Shapes `[2,3]`, `[3,2]`, and `[6]` can be
paired because each flattens to six entries. No broadcasting occurs: a
one-element operand is not repeated, and matching trailing dimensions do not
help when total element counts differ.

## Parameters

**other** (`const Series&`)

The second operand. After both are flattened, the element counts must match.

## Returns

`Series`
    A scalar (`shape=[]`) holding the inner product.

## Gradient

`inner()` is differentiable in both operands: `d/da = g * b`, `d/db = g * a`
(with `g` the scalar upstream sensitivity).

Each sensitivity is reshaped back to its operand's original shape. Differently
shaped operands can therefore be paired without losing their own geometry.

## Examples

Example 1. Two vectors contracted to a scalar.

```cpp
auto a = pluss::make_series({1.0, 2.0});
auto b = pluss::make_series({3.0, 4.0});

std::cout << a.inner(b);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=11
```

`1*3 + 2*4 = 11`.

Example 2. Non-vector operands are flattened first.

```cpp
auto m = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto n = pluss::make_series({{1.0, 0.0}, {0.0, 1.0}});

std::cout << m.inner(n);
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
```

`[1,2,3,4] · [1,0,0,1] = 1 + 4 = 5` — the trace of `m`, in this case.

## Gotchas And Failure Modes

- Both operands are flattened; a shape difference that survives flattening (a
  different total element count) is an error.
- The result is always a scalar `Series`, not a `double`. Call `.item<double>()`
  to extract the number.
- For a strict 1-D dot product without flattening, use
  [`SeriesLinearAlgAccessor::dot`](../series-linearalg/dot.md).

## Summary

`inner(other)` is the flattened sum of elementwise products:

- both operands flattened, element counts must match
- scalar `Series` result; differentiable in both operands
- the scalar contraction that complements [`outer()`](outer.md)

## See Also

- [`Series::outer`](outer.md): the rank-1 expansion.
- [`SeriesLinearAlgAccessor::dot`](../series-linearalg/dot.md): strict 1-D dot
  product.
- [`Series::mul`](mul.md) + [`Series::sum`](sum.md): the explicit two-step form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index_select.html">
    <span class="label">Previous</span>
    <span class="title">index_select</span>
  </a>
  <a class="next-link" href="intersect1d.html">
    <span class="label">Next</span>
    <span class="title">intersect1d</span>
  </a>
</div>
