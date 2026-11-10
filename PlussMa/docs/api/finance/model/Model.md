<!-- generated from Model.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `module::Model`

A value holding one of the six model families, with its parameters as a flat vector.

## Signature

```cpp
module::Model::Model(const module::ModelVariant &value);
```

## Construction

Build one from plain numbers with [`Model::make`](Model/make.md) (`fin::model::make` is the same function), or construct it from a parameter struct: `mod::Model m = fin::model::Heston{...};`. A default `Model` holds an empty Black-Scholes struct (its volatility is undefined, so it fails validation until set).

## Methods

| Method | What it does |
|---|---|
| [`as`](Model/as.md) | The held struct of a given family, for reading or editing; throws if the family is different. |
| [`holds`](Model/holds.md) | Whether the model currently holds a given family struct. |
| [`kind`](Model/kind.md) | The family the model holds, as a `Kind`. |
| [`make`](Model/make.md) | Builds a model of a given kind from a plain list of numbers, validating it. |
| [`name`](Model/name.md) | The name of the held model family as text. |
| [`parameters`](Model/parameters.md) | Reads the parameters as a flat vector, or sets them from a vector, a `Series`, a span, an array or a `std::vector`. |
| [`validate`](Model/validate.md) | Checks that every parameter is defined, finite and inside its domain. |
| [`visit`](Model/visit.md) | Applies a function to whichever struct the model holds. |

## Changelog

- **0.1**: initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Model/Introduction
Model/as
Model/holds
Model/kind
Model/make
Model/name
Model/parameters
Model/validate
Model/visit
```
<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Kind.html">
    <span class="label">Previous</span>
    <span class="title">Kind</span>
  </a>
  <a class="next-link" href="RoughBergomi.html">
    <span class="label">Next</span>
    <span class="title">RoughBergomi</span>
  </a>
</div>
<!-- apiref:nav:end -->
