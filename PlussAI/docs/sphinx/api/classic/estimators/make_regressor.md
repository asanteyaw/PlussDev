# `pluss::ai::classic::make_regressor`

`make_regressor` turns a [`ModelSpec`](../families/ModelSpec.md) into a real,
untrained [`RegressorBase`](RegressorBase.md). It is the only way to obtain
one.

## Signature

```cpp
classic::StatusOr<std::unique_ptr<classic::RegressorBase>> classic::make_regressor(
    const classic::ModelSpec& spec);
```

## Parameters

`spec` — `ModelSpec`
: Must have `.task == TaskKind::Regression`, and `.options` must hold the
  `*FamilyOptions` alternative matching `.family`. See
  [`ModelSpec`](../families/ModelSpec.md).

## Returns

A `StatusOr<std::unique_ptr<RegressorBase>>`. On success, an untrained
regressor — unlike [`make_classifier`](make_classifier.md), every
`LinearFamilyOptions::objective` value (`LeastSquares`, `RobustLeast`, and
even `Logistic`/`Hinge` if a caller sets them) is accepted here at
construction time; the regression fit kernels validate `objective`
themselves at [`learn`](EstimatorBase/learn.md) time the same way
classification does, just with the opposite pair of accepted values.

## Errors

- `Status::invalid_argument` — `spec.task != TaskKind::Regression`.
- `Status::invalid_argument` — `spec.family` does not match the alternative
  active in `spec.options`.

## Practical Examples

### Example 1

One spec per regression-capable family, each built successfully.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::ModelSpec linear_spec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                   cls::LinearFamilyOptions{}};
  const cls::ModelSpec tree_spec{cls::TaskKind::Regression, cls::ModelFamily::Tree, cls::TreeFamilyOptions{}};
  const cls::ModelSpec ensemble_spec{cls::TaskKind::Regression, cls::ModelFamily::Ensemble,
                                     cls::EnsembleFamilyOptions{}};

  std::cout << "linear ok   = " << std::boolalpha << cls::make_regressor(linear_spec).ok() << "\n";
  std::cout << "tree ok     = " << cls::make_regressor(tree_spec).ok() << "\n";
  std::cout << "ensemble ok = " << cls::make_regressor(ensemble_spec).ok() << "\n";

  return 0;
}
```

Output

```text
linear ok   = true
tree ok     = true
ensemble ok = true
```

### Example 2

Building, fitting, and predicting in one straight line — the full path from
`ModelSpec` to a prediction.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made = cls::make_regressor(
      cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear, cls::LinearFamilyOptions{}});

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0, 3.0}, {3}, f64).reshape({3, 1});
  batch.y = pluss::make_series({2.0, 4.0, 6.0}, {3}, f64);
  made.value()->learn(batch);

  cls::FeatureBatch probe{};
  probe.x = pluss::make_series({4.0}, {1}, f64).reshape({1, 1});
  std::cout << "prediction = " << made.value()->infer(probe).value().item<double>() << "\n";

  return 0;
}
```

Output

```text
prediction = 8
```

### Example 3

Both construction-time failure modes: a `Classification`-task spec, and a
`.family` / `.options` mismatch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  const cls::ModelSpec wrong_task{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear};
  const cls::StatusOr<std::unique_ptr<cls::RegressorBase>> s1 = cls::make_regressor(wrong_task);
  std::cout << "wrong task: ok = " << std::boolalpha << s1.ok() << ", message = \""
            << s1.status().message << "\"\n";

  cls::ModelSpec wrong_options{};
  wrong_options.task = cls::TaskKind::Regression;
  wrong_options.family = cls::ModelFamily::Kernel;
  wrong_options.options = cls::EnsembleFamilyOptions{};
  const cls::StatusOr<std::unique_ptr<cls::RegressorBase>> s2 = cls::make_regressor(wrong_options);
  std::cout << "wrong options: ok = " << s2.ok() << ", message = \"" << s2.status().message << "\"\n";

  return 0;
}
```

Output

```text
wrong task: ok = false, message = "make_regressor requires ModelSpec.task = TaskKind::Regression."
wrong options: ok = false, message = "Kernel regressor requires KernelFamilyOptions in ModelSpec.options."
```

## See Also

- [`RegressorBase`](RegressorBase.md) — what you get back.
- [`ModelSpec`](../families/ModelSpec.md) — the input, and every way it can be
  malformed.
- [`make_classifier`](make_classifier.md) / [`make_transformer`](make_transformer.md) —
  the other two factories.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
