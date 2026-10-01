<!-- pluss-hand-authored -->

# Series::positive

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::positive() const -> Series
Series& Series::positive_() const
```

## What It Does

`positive()` is unary plus: it returns `self` unchanged. There is no
computation — no sign flip, no clamping, no dtype change. It exists purely
for API symmetry with [`neg()`](neg.md), the way a language's own unary `+`
operator mirrors unary `-`.

`positive_()` is the in-place spelling: it is also a pure no-op, returning a
reference to `self` without touching its storage or bumping its version.

Since neither form has a real out-of-place/in-place mutation to distinguish,
both are documented together on this one page rather than split.

## Parameters

None. This is a zero-argument identity on `self`.

## Returns

`Series`
    The out-of-place form returns `self` itself (not a copy) — the identity
    result of a no-op.

`Series&`
    The in-place form returns a reference to `self`, unmodified.

## Gradient

Identity is trivially differentiable: `d/dx positive(x) = 1`. In practice
this rarely matters, since `positive()`/`positive_()` don't change anything
a real computation would depend on.

## Examples

Example 1. `positive()` returns the same values, unchanged.

```cpp
auto x = pluss::make_series({-1.0, 0.0, 2.0});
std::cout << x.positive();
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[-1, 0, 2]
```

Example 2. `positive_()` is a genuine no-op: same values, same object.

```cpp
auto x = pluss::make_series({-1.0, 2.0});

auto& ref = x.positive_();
std::cout << "x=" << x << "\n";
std::cout << std::boolalpha << "same object: " << (&ref == &x);
```

### Output

```text
x=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[-1, 2]
same object: true
```

## Gotchas And Failure Modes

- Unlike [`neg()`](neg.md), `positive()` never changes sign — including for
  negative values. It is not "absolute value."
- Because it's a true no-op, don't reach for `positive()`/`positive_()`
  expecting any normalization or validation side effect.

## Summary

`positive()` / `positive_()` are identity no-ops kept for symmetry with
`neg()` / `neg_()`. Nothing about the values, dtype, or shape changes.

## See Also

- [`Series::neg`](neg.md): the actual sign-flipping counterpart.
- [`Series::abs`](abs.md): discards sign entirely, rather than preserving it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="polygamma.html">
    <span class="label">Previous</span>
    <span class="title">polygamma</span>
  </a>
  <a class="next-link" href="pos.html">
    <span class="label">Next</span>
    <span class="title">pos</span>
  </a>
</div>
