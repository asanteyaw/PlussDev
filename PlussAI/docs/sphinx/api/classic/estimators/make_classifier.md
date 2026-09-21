# `pluss::ai::classic::make_classifier`

`make_classifier` turns a [`ModelSpec`](../families/ModelSpec.md) into a real,
untrained [`ClassifierBase`](ClassifierBase.md). It is the only way to obtain
one.

## Signature

```cpp
classic::StatusOr<std::unique_ptr<classic::ClassifierBase>> classic::make_classifier(
    const classic::ModelSpec& spec);
```

## Parameters

`spec` — `ModelSpec`
: Must have `.task == TaskKind::Classification`, and `.options` must hold the
  `*FamilyOptions` alternative matching `.family`. See
  [`ModelSpec`](../families/ModelSpec.md).

## Returns

A `StatusOr<std::unique_ptr<ClassifierBase>>`. On success, an untrained
classifier — `.value()->trained()` is `false` until you call
[`learn`](EstimatorBase/learn.md).

## Errors

- `Status::invalid_argument` — `spec.task != TaskKind::Classification`.
- `Status::invalid_argument` — `spec.family` does not match the alternative
  active in `spec.options`.
- Construction never checks family-internal option validity beyond the shape
  match above — a `Linear` classifier spec with `objective = LeastSquares`
  (invalid for classification) constructs successfully here and only fails at
  the first [`learn`](EstimatorBase/learn.md) call — see
  [`LinearFamilyOptions`](../families/LinearFamilyOptions.md#objective).

## Practical Examples

### Example 1

One spec per classification-capable family, each built successfully.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;

  const cls::ModelSpec linear_spec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear};
  const cls::ModelSpec tree_spec{cls::TaskKind::Classification, cls::ModelFamily::Tree, cls::TreeFamilyOptions{}};
  const cls::ModelSpec bayes_spec{cls::TaskKind::Classification, cls::ModelFamily::Bayes, cls::BayesFamilyOptions{}};

  std::cout << "linear ok = " << std::boolalpha << cls::make_classifier(linear_spec).ok() << "\n";
  std::cout << "tree ok   = " << cls::make_classifier(tree_spec).ok() << "\n";
  std::cout << "bayes ok  = " << cls::make_classifier(bayes_spec).ok() << "\n";

  return 0;
}
```

Output

```text
linear ok = true
tree ok   = true
bayes ok  = true
```

### Example 2

A newly-constructed classifier is untrained, and stays that way until
`learn` succeeds.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Hinge;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  std::cout << "made.ok() = " << std::boolalpha << made.ok() << "\n";
  std::cout << "trained() right after construction = " << made.value()->trained() << "\n";

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  batch.y = pluss::make_series({0.0, 1.0}, {2}, f64);
  made.value()->learn(batch);
  std::cout << "trained() after learn()            = " << made.value()->trained() << "\n";

  return 0;
}
```

Output

```text
made.ok() = true
trained() right after construction = false
trained() after learn()            = true
```

### Example 3

Both construction-time failure modes: a `Regression`-task spec, and a
`.family` / `.options` mismatch.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  const cls::ModelSpec wrong_task{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                  cls::LinearFamilyOptions{}};
  const cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> s1 = cls::make_classifier(wrong_task);
  std::cout << "wrong task: ok = " << std::boolalpha << s1.ok() << ", message = \""
            << s1.status().message << "\"\n";

  cls::ModelSpec wrong_options{};
  wrong_options.task = cls::TaskKind::Classification;
  wrong_options.family = cls::ModelFamily::Bayes;
  wrong_options.options = cls::TreeFamilyOptions{};
  const cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> s2 = cls::make_classifier(wrong_options);
  std::cout << "wrong options: ok = " << s2.ok() << ", message = \"" << s2.status().message << "\"\n";

  return 0;
}
```

Output

```text
wrong task: ok = false, message = "make_classifier requires ModelSpec.task = TaskKind::Classification."
wrong options: ok = false, message = "Bayes classifier requires BayesFamilyOptions in ModelSpec.options."
```

## See Also

- [`ClassifierBase`](ClassifierBase.md) — what you get back.
- [`ModelSpec`](../families/ModelSpec.md) — the input, and every way it can be
  malformed.
- [`make_regressor`](make_regressor.md) / [`make_transformer`](make_transformer.md) —
  the other two factories.

<!-- apiref:nav:start -->
<!-- apiref:nav:end -->
