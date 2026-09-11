# `pluss::ai::classic::DecomposeFamilyOptions`

`DecomposeFamilyOptions` configures a dimensionality-reduction estimator —
currently PCA: how many components to keep, whether to whiten them, and the
iterative solver's limits.

## Fields

### `method`

`DecomposeMethod method = DecomposeMethod::Pca`

The decomposition algorithm. `Pca` (principal component analysis) is the only
value today.

### `components`

`std::int64_t components = -1`

How many components to keep. `-1` (the default) keeps all of them (equal to
the input feature count) — a full change of basis with no dimensionality
reduction. A positive value keeps only that many top components, ordered by
explained variance — the usual way to actually reduce dimensionality.

### `whiten`

`bool whiten = false`

`false` (the default): components are scaled by their explained variance, so
earlier components have larger magnitude. `true`: each component is rescaled
to unit variance after projection — useful when a downstream estimator is
sensitive to feature scale (many are) and you want the PCA output pre-scaled
for it.

### `max_iterations`

`std::int64_t max_iterations = 500`

Iteration cap for the iterative solver. `> 0`. A small or well-conditioned
input may converge in a closed-form-equivalent way well under this cap; it
only matters for large or ill-conditioned inputs.

### `tolerance`

`double tolerance = 1e-6`

Convergence tolerance for the iterative solver. `> 0`.

## Guidance

Set `components` explicitly whenever the goal is actual dimensionality
reduction — the `-1` default performs a full, lossless change of basis, which
is the right choice for whitening or basis-rotation use cases but not for
compression. Turn `whiten = true` on specifically when the PCA output feeds a
distance- or scale-sensitive downstream estimator ([`NeighborFamilyOptions`](NeighborFamilyOptions.md)'s
family, most kernel methods).

## Practical Example

`components` reducing a 3-feature input down to its top 1 or 2 components.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  // Three correlated features -- most of the variance lives in one direction.
  const cls::Series x = pluss::make_series(
      {1.0, 2.0, 1.0, 2.0, 4.0, 2.1, 3.0, 6.0, 3.2, 4.0, 8.0, 3.9},
      {12}, f64).reshape({4, 3});
  cls::FeatureBatch batch{};
  batch.x = x;

  auto fit_and_map = [&](std::int64_t components) {
    cls::DecomposeFamilyOptions opts{};
    opts.components = components;
    cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made = cls::make_transformer(
        cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Decompose, opts});
    made.value()->learn(batch);
    return made.value()->map(batch).value();
  };

  std::cout << "components = 1 ->\n" << fit_and_map(1) << "\n";
  std::cout << "components = 2 ->\n" << fit_and_map(2) << "\n";

  return 0;
}
```

Output

```text
PENDING_VERIFICATION_DECOMPOSEFAMILYOPTIONS_1
```

## Changelog

- **0.1** — `method`, `components`, `whiten`, `max_iterations`, `tolerance`.

## See Also

- [`ModelSpec`](ModelSpec.md) — carries this as one `FamilyOptions`
  alternative; note the `Transform` task.
- [`prep`](../prep.md) — feature-level transforms, for preprocessing before a
  decomposition rather than after one.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
