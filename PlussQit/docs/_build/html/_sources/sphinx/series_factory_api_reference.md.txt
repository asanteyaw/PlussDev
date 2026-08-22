<!-- pluss-hand-authored -->

# Series Factory API Reference

[Back to PlussQit API Reference](../api_reference.md)

This page documents the `pluss::series::*` factory surface separately from the
[`Series` method reference](series_api_reference.md).

That split is intentional.

- The method reference answers: "I already have a `Series`; what can I do to it?"
- The factory reference answers: "I do not have a `Series` yet; how do I build
  the right one?"

Those are different jobs, and mixing them on one landing page makes both harder
to navigate.

## What Lives Here

`pluss::series::*` is the construction namespace for numeric/boolean `Series`.

Use it for:

- direct value construction with `make`
- shape-first allocation like `empty`, `zeros`, `ones`, `full`
- range/grid builders like `arange`, `linspace`, `eye`
- random and distribution factories like `rand`, `randn`, `randint`,
  `bernoulli`, `poisson`, `multinomial`
- like-factories such as `zeros_like` and `randn_like`
- complex constructors such as `complex` and `polar`

## Reading the Overloads

Many Series factories look wider than they really are because several public
spellings funnel into the same underlying configuration path.

For the core families, the overload story usually collapses into one of three
ideas:

- explicit dtype: `pluss::series::zeros({2, 3}, pluss::ScalarType::Float64)`
- explicit config: `pluss::series::zeros({2, 3}, pluss::Config{}.with_dtype(...))`
- option-style sugar: typed tags or key/value config lists that ultimately build
  the same `Config`

So the handwritten pages document the canonical behavior first, then call out
the overload quirks that materially change the result.

## Current Coverage

This first handwritten factory batch covers the core constructors users touch
immediately:

- `arange`
- `empty`
- `full`
- `make`
- `ones`
- `zeros`

The next batch should extend this landing page into `eye`, `linspace`, the
stochastic families, like-factories, and the complex constructors.

```{toctree}
:hidden:
:maxdepth: 1

api/series_factory/arange
api/series_factory/empty
api/series_factory/full
api/series_factory/make
api/series_factory/ones
api/series_factory/zeros
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
  <a class="next-link" href="api/series_factory/arange.html">
    <span class="label">Next</span>
    <span class="title">arange</span>
  </a>
</div>
