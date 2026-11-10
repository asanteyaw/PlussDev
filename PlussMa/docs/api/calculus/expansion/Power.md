<!-- generated from Power.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::expansion::Power`

Termwise calculus on coefficient vectors: derivative, integral and product of power series.

## Signature

```cpp
operators::expansion::Power operators::expansion::power();
```

## Construction

`Power` is returned by the no-argument [`power()`](power_operator.md). It has no data and is cheap to create and copy.

## Methods

| Method | What it does |
|---|---|
| [`derivative`](Power/derivative.md) | Differentiates a power series termwise: `c′ₖ = (k + 1) c_{k+1}`. |
| [`integral`](Power/integral.md) | Integrates a power series termwise from the centre: `c′₀ = 0`, `c′_{k+1} = cₖ/(k + 1)`. |
| [`product`](Power/product.md) | Multiplies two power series (the Cauchy product), truncated to the shorter length. |

## Changelog

- **0.1**: initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Power/Introduction
Power/derivative
Power/integral
Power/product
```
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="Taylor.html">
    <span class="label">Next</span>
    <span class="title">Taylor</span>
  </a>
</div>
<!-- apiref:nav:end -->
