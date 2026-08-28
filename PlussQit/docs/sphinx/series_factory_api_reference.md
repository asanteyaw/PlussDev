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
- public rendering through `print`
- ordered conditional construction through `select`
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

The current handwritten factory batch now covers the public constructor families
most users actually reach first and second, plus the free-function helpers that
show up immediately after construction:

- direct value construction: `make`
- free-function helpers on built `Series`: `print`, `select`
- shape-first constants and layouts: `empty`, `zeros`, `ones`, `full`, `eye`
- range/grid builders: `arange`, `linspace`
- like-factories: `empty_like`, `zeros_like`, `ones_like`, `full_like`,
  `rand_like`, `randn_like`, `randint_like`, `eye_like`
- stochastic constructors: `rand`, `randn`, `rand_btw`, `randint`, `normal`,
  `randperm`, `bernoulli`, `poisson`, `multinomial`
- complex constructors: `complex`, `polar`

Those pages are split this way on purpose.

- The core random pages (`rand`, `randn`) teach seed and dtype policy.
- The interval/discrete pages (`rand_btw`, `randint`, `randperm`) teach how
  those rules change once bounds or uniqueness matter.
- The like-factory pages teach "same geometry as this existing tensor" as a
  separate mental model from ordinary shape-first construction.
- The complex pages teach the pairwise "two real tensors in, one complex tensor
  out" surface, which behaves differently from the scalar-fill factories.
- The helper pages (`print`, `select`) live here because they are free
  functions on `pluss::series::*`, not `Series` methods.

What is still out of scope for this handwritten batch is the unsupported sparse
and quantized constructor stub family.

```{toctree}
:hidden:
:maxdepth: 1

api/series_factory/arange
api/series_factory/empty
api/series_factory/eye
api/series_factory/full
api/series_factory/linspace
api/series_factory/make
api/series_factory/print
api/series_factory/select
api/series_factory/ones
api/series_factory/zeros
api/series_factory/rand
api/series_factory/rand_btw
api/series_factory/randint
api/series_factory/randn
api/series_factory/normal
api/series_factory/randperm
api/series_factory/empty_like
api/series_factory/zeros_like
api/series_factory/ones_like
api/series_factory/full_like
api/series_factory/rand_like
api/series_factory/randn_like
api/series_factory/randint_like
api/series_factory/eye_like
api/series_factory/bernoulli
api/series_factory/poisson
api/series_factory/multinomial
api/series_factory/complex
api/series_factory/polar
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
