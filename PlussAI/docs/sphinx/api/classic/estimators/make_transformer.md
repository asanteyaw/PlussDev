# `pluss::ai::classic::make_transformer`

`make_transformer` turns a [`ModelSpec`](../families/ModelSpec.md) into a
real, untrained [`TransformerBase`](TransformerBase.md). It is the only way
to obtain one.

## Signature

```cpp
classic::StatusOr<std::unique_ptr<classic::TransformerBase>> classic::make_transformer(
    const classic::ModelSpec& spec);
```

## What It Does

The transformer counterpart to
[`make_classifier`](make_classifier.md)/[`make_regressor`](make_regressor.md):
reads `spec.family` and dispatches to that family's concrete transformer
implementation. Only the four families with a genuine transformer form
(`Cluster`, `Decompose`, `Detect`, one-class `Kernel`) are supported --
`spec.family` outside that set is rejected rather than silently accepted.
The returned object is untrained until [`learn`](EstimatorBase/learn.md)
runs.

## Parameters

`spec` — `ModelSpec`
: Must have `.task == TaskKind::Transform`, and `.family` must be one of the
  four families this function actually supports:
  [`Cluster`](../families/ClusterFamilyOptions.md),
  [`Decompose`](../families/DecomposeFamilyOptions.md),
  [`Detect`](../families/DetectFamilyOptions.md), or
  [`Kernel`](../families/KernelFamilyOptions.md) (specifically its one-class
  form — `KernelFamilyOptions::form = OneClass`). The other five families
  (`Linear`, `Tree`, `Ensemble`, `Neighbor`, `Bayes`) have no transformer form
  and are rejected here — reach them through
  [`make_classifier`](make_classifier.md) / [`make_regressor`](make_regressor.md)
  instead. `.options` must still hold the alternative matching `.family`.

## Returns

A `StatusOr<std::unique_ptr<TransformerBase>>`. On success, an untrained
transformer.

## Errors

- `Status::invalid_argument` — `spec.task != TaskKind::Transform`.
- `Status::invalid_argument` — `spec.family` is one of the five
  transformer-incapable families (`Linear`, `Tree`, `Ensemble`, `Neighbor`,
  `Bayes`).
- `Status::invalid_argument` — `spec.family` is one of the four
  transformer-capable families, but `spec.options` holds a different
  alternative.

## Practical Examples

### Example 1

The four transformer-capable families, each built successfully.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::ModelSpec kmeans{cls::TaskKind::Transform, cls::ModelFamily::Cluster, cls::ClusterFamilyOptions{}};
  const cls::ModelSpec pca{cls::TaskKind::Transform, cls::ModelFamily::Decompose, cls::DecomposeFamilyOptions{}};
  const cls::ModelSpec isolation{cls::TaskKind::Transform, cls::ModelFamily::Detect, cls::DetectFamilyOptions{}};

  cls::KernelFamilyOptions one_class{};
  one_class.form = cls::KernelForm::OneClass;
  const cls::ModelSpec kernel{cls::TaskKind::Transform, cls::ModelFamily::Kernel, one_class};

  std::cout << "kmeans ok    = " << std::boolalpha << cls::make_transformer(kmeans).ok() << "\n";
  std::cout << "pca ok       = " << cls::make_transformer(pca).ok() << "\n";
  std::cout << "isolation ok = " << cls::make_transformer(isolation).ok() << "\n";
  std::cout << "one-class svm ok = " << cls::make_transformer(kernel).ok() << "\n";

  return 0;
}
```

Output

```text
kmeans ok    = true
pca ok       = true
isolation ok = true
one-class svm ok = true
```

### Example 2

`.task` is checked, and so is `.family` — a `Classification`-task spec over
`Linear` fails both ways at once (wrong task; `Linear` also has no
transformer form at all).

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec wrong_task{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear};

  const cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made = cls::make_transformer(wrong_task);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  const cls::ModelSpec right_task_wrong_family{cls::TaskKind::Transform, cls::ModelFamily::Linear, linear};
  const cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made2 = cls::make_transformer(right_task_wrong_family);
  std::cout << "task fixed, family still Linear: ok = " << made2.ok() << ", message = \""
            << made2.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "make_transformer requires ModelSpec.task = TaskKind::Transform."
task fixed, family still Linear: ok = false, message = "Unsupported transformer family; expected Cluster, Decompose, Detect, or Kernel(one-class)."
```

### Example 3

`.family` / `.options` mismatch, between two transformer-capable families.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ModelSpec mismatched{};
  mismatched.task = cls::TaskKind::Transform;
  mismatched.family = cls::ModelFamily::Cluster;
  mismatched.options = cls::DecomposeFamilyOptions{};  // says Cluster, holds Decompose

  const cls::StatusOr<std::unique_ptr<cls::TransformerBase>> made = cls::make_transformer(mismatched);
  std::cout << "ok = " << std::boolalpha << made.ok() << "\n";
  std::cout << "message = \"" << made.status().message << "\"\n";

  return 0;
}
```

Output

```text
ok = false
message = "Cluster transformer requires ClusterFamilyOptions in ModelSpec.options."
```

## See Also

- [`TransformerBase`](TransformerBase.md) — what you get back.
- [`ModelSpec`](../families/ModelSpec.md) — the input.
- [`make_classifier`](make_classifier.md) / [`make_regressor`](make_regressor.md) —
  the other two factories, for the five families with no transformer form.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
