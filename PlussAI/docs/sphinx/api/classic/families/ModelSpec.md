# `pluss::ai::classic::ModelSpec`

`ModelSpec` is the declarative description of one estimator configuration —
what task it solves, which family implements it, and that family's options —
without actually building anything. [`make_classifier`](../estimators/make_classifier.md) /
[`make_regressor`](../estimators/make_regressor.md) /
[`make_transformer`](../estimators/make_transformer.md) turn a `ModelSpec`
into a real estimator; [`sweep`](../tune/sweep.md) / [`tune`](../tune/tune.md)
rank a whole list of them without building any until the winner is chosen.
Describing a model as data before building it is what makes tuning and
comparison possible without a combinatorial explosion of estimator objects.

## Signature

```cpp
using classic::FamilyOptions = std::variant<
    classic::LinearFamilyOptions, classic::TreeFamilyOptions, classic::EnsembleFamilyOptions,
    classic::NeighborFamilyOptions, classic::KernelFamilyOptions, classic::BayesFamilyOptions,
    classic::ClusterFamilyOptions, classic::DecomposeFamilyOptions, classic::DetectFamilyOptions>;
```

## Construction

`ModelSpec` is a plain aggregate — three fields, no constructors beyond
aggregate initialization:

```cpp
cls::ModelSpec spec{};
spec.task = cls::TaskKind::Classification;
spec.family = cls::ModelFamily::Linear;
spec.options = cls::LinearFamilyOptions{};        // must match .family
```

or positionally: `cls::ModelSpec{cls::TaskKind::Classification,
cls::ModelFamily::Linear, cls::LinearFamilyOptions{}}`.

## State

`task` — `TaskKind`
: `Classification`, `Regression`, or `Transform`. Determines which
  `make_*` function accepts the spec: `make_classifier` requires
  `Classification`, `make_regressor` requires `Regression`, `make_transformer`
  accepts any task (a transformer has no notion of a label). Default:
  `Regression`.

`family` — `ModelFamily`
: Which of the nine estimator families implements this spec: `Linear`,
  `Tree`, `Ensemble`, `Neighbor`, `Kernel`, `Bayes`, `Cluster`, `Decompose`,
  `Detect`. Default: `Linear`. This value and the *active* alternative held in
  `.options` must agree — `family = Tree` with `.options` holding a
  `LinearFamilyOptions` is a mismatched spec (see Errors below).

`options` — `FamilyOptions` (`std::variant` of all nine `*FamilyOptions`
structs)
: The chosen family's construction options. Default: a default-constructed
  `LinearFamilyOptions` (matching the default `family = Linear`). See each
  family's own options page — [`LinearFamilyOptions`](LinearFamilyOptions.md),
  [`TreeFamilyOptions`](TreeFamilyOptions.md), and the rest — for every field.

## Methods

None — `ModelSpec` is three public fields, read and set directly. It exists to
be passed to [`make_classifier`](../estimators/make_classifier.md) /
[`make_regressor`](../estimators/make_regressor.md) /
[`make_transformer`](../estimators/make_transformer.md), or collected into a
list for [`sweep`](../tune/sweep.md) / [`tune`](../tune/tune.md); it has no
behavior of its own.

## Errors

`ModelSpec` itself never validates anything — it is inert data. The mismatch
between `.family` and the alternative active in `.options` is only caught
when the spec is actually used: `make_classifier` / `make_regressor` /
`make_transformer` return `Status::invalid_argument` for a mismatched spec,
and `make_classifier` / `make_regressor` additionally reject a `.task` that
does not match the function (`make_classifier` on a `Regression`-task spec,
for instance).

One family/task combination is checked later still, at `learn(...)` rather
than at construction: a `Classification`-task `ModelFamily::Linear` spec whose
`LinearFamilyOptions::objective` is the default `LeastSquares` (or
`RobustLeast`) builds successfully but fails the first `learn` call — see
[`LinearFamilyOptions`](LinearFamilyOptions.md#objective).

## Practical Examples

### Example 1

One spec per family shape, each built and handed to the matching `make_*`
function — the family/task combinations that succeed.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::ModelSpec linear_clf{cls::TaskKind::Classification, cls::ModelFamily::Linear,
                                  cls::LinearFamilyOptions{}};
  const cls::ModelSpec tree_reg{cls::TaskKind::Regression, cls::ModelFamily::Tree,
                                cls::TreeFamilyOptions{}};
  const cls::ModelSpec kmeans{cls::TaskKind::Transform, cls::ModelFamily::Cluster,
                              cls::ClusterFamilyOptions{}};

  std::cout << "linear classifier made ok = " << std::boolalpha
            << cls::make_classifier(linear_clf).ok() << "\n";
  std::cout << "tree regressor made ok    = " << cls::make_regressor(tree_reg).ok() << "\n";
  std::cout << "kmeans transformer made ok = " << cls::make_transformer(kmeans).ok() << "\n";

  return 0;
}
```

Output

```text
PENDING_VERIFICATION_MODELSPEC_1
```

### Example 2

Two ways a spec can be wrong: `.task` mismatched with the `make_*` function,
and `.family` mismatched with the active `.options` alternative.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::ModelSpec regression_spec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                       cls::LinearFamilyOptions{}};
  const cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> wrong_task =
      cls::make_classifier(regression_spec);
  std::cout << "regression spec into make_classifier: ok = " << std::boolalpha << wrong_task.ok()
            << ", message = \"" << wrong_task.status().message << "\"\n";

  cls::ModelSpec mismatched{};
  mismatched.task = cls::TaskKind::Classification;
  mismatched.family = cls::ModelFamily::Tree;         // says Tree...
  mismatched.options = cls::LinearFamilyOptions{};     // ...but options holds Linear
  const cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> wrong_family =
      cls::make_classifier(mismatched);
  std::cout << "mismatched family/options: ok = " << wrong_family.ok() << ", message = \""
            << wrong_family.status().message << "\"\n";

  return 0;
}
```

Output

```text
PENDING_VERIFICATION_MODELSPEC_2
```

### Example 3

Building a small candidate list of specs — the shape [`sweep`](../tune/sweep.md)
and [`tune`](../tune/tune.md) consume — without building any estimator until
one is chosen.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>
#include <vector>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions light{};
  light.regularization_strength = 1e-6;
  cls::LinearFamilyOptions heavy{};
  heavy.regularization_strength = 1e2;

  const std::vector<cls::ModelSpec> candidates = {
      {cls::TaskKind::Regression, cls::ModelFamily::Linear, light},
      {cls::TaskKind::Regression, cls::ModelFamily::Linear, heavy},
      {cls::TaskKind::Regression, cls::ModelFamily::Tree, cls::TreeFamilyOptions{}},
  };

  std::cout << "candidate count = " << candidates.size() << "\n";
  for (const cls::ModelSpec& spec : candidates) {
    std::cout << "  family = " << static_cast<int>(spec.family)
              << ", task = " << static_cast<int>(spec.task) << "\n";
  }

  return 0;
}
```

Output

```text
PENDING_VERIFICATION_MODELSPEC_3
```

## Changelog

- **0.1** — `task` + `family` + `options` (a nine-way variant).

## See Also

- [`estimators`](../estimators.md) — `make_classifier` / `make_regressor` /
  `make_transformer`, the functions that consume a `ModelSpec`.
- Every `*FamilyOptions` page — [`LinearFamilyOptions`](LinearFamilyOptions.md),
  [`TreeFamilyOptions`](TreeFamilyOptions.md),
  [`EnsembleFamilyOptions`](EnsembleFamilyOptions.md),
  [`NeighborFamilyOptions`](NeighborFamilyOptions.md),
  [`KernelFamilyOptions`](KernelFamilyOptions.md),
  [`BayesFamilyOptions`](BayesFamilyOptions.md),
  [`ClusterFamilyOptions`](ClusterFamilyOptions.md),
  [`DecomposeFamilyOptions`](DecomposeFamilyOptions.md),
  [`DetectFamilyOptions`](DetectFamilyOptions.md).
- [`tune`](../tune.md) — ranking a list of `ModelSpec` candidates.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
