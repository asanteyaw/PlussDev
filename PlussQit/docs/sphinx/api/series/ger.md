<!-- pluss-hand-authored -->

# Series::ger

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::ger(const Series& other) const
```

## What It Does

`ger()` is an exact alias of [`outer()`](outer.md) — the BLAS spelling of the
vector outer product (`ger` = "general rank-1"). The receiver is `[m]`, `other`
is `[n]`, the result is the `[m, n]` matrix with `(i, j)` entry `a[i] * b[j]`.

See [`outer()`](outer.md) for the full description, gradient rules, and
examples.

## Parameters

**other** (`const Series&`) — the `[n]` second vector.

## Returns

`Series`
    The `[m, n]` outer-product matrix. Differentiable in both operands.

## Example

```cpp
auto a = pluss::make_series({1.0, 2.0});
auto b = pluss::make_series({3.0, 4.0, 5.0});

std::cout << a.ger(b);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 4, 5],
  [6, 8, 10]]
```

## See Also

- [`Series::outer`](outer.md): the primary page for this operation.
- [`Series::addr`](addr.md): the accumulating form `M + alpha * (a ⊗ b)`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="geometric_.html">
    <span class="label">Previous</span>
    <span class="title">geometric_</span>
  </a>
  <a class="next-link" href="gelu.html">
    <span class="label">Next</span>
    <span class="title">gelu</span>
  </a>
</div>
