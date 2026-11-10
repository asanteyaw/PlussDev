<!-- generated from Taylor.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::expansion::Taylor`

The companions of the Taylor operator: the coefficient vector and the Lagrange remainder.

## Signature

```cpp
operators::expansion::Taylor operators::expansion::taylor();
```

## Construction

`Taylor` is returned by the no-argument [`taylor()`](taylor_operator.md). It has no data and is cheap to create and copy.

## Methods

| Method | What it does |
|---|---|
| [`coefficients`](Taylor/coefficients.md) | The Taylor coefficients `f⁽ᵏ⁾(a)/k!` as a vector function, rather than as a polynomial. |
| [`remainder`](Taylor/remainder.md) | The Lagrange remainder factor `f⁽ⁿ⁺¹⁾(x)/(n+1)!`, which bounds the error of the Taylor polynomial. |

## Changelog

- **0.1**: initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Taylor/Introduction
Taylor/coefficients
Taylor/remainder
```
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Power.html">
    <span class="label">Previous</span>
    <span class="title">Power</span>
  </a>
  <a class="next-link" href="power_operator.html">
    <span class="label">Next</span>
    <span class="title">power</span>
  </a>
</div>
<!-- apiref:nav:end -->
