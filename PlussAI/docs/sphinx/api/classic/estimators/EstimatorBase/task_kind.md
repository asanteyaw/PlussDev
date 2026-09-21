<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: af0475b4d4d5 -->
# `pluss::ai::classic::EstimatorBase::task_kind`

`task_kind` reports which task the estimator solves. It is fixed at
construction — [`ClassifierBase`](../ClassifierBase.md) always answers
`Classification`, [`RegressorBase`](../RegressorBase.md) always answers
`Regression` — and never changes, including across `learn` calls.

## Signature

```cpp
classic::TaskKind classic::EstimatorBase::task_kind() const noexcept;
```

## Parameters

None.

## Returns

The `TaskKind` (`Classification` or `Regression`) fixed by which base class
the concrete estimator derives from — independent of whether the estimator is
trained.

## Errors

None — `noexcept`, always succeeds.

## Practical Examples

### Example 1

Reading `task_kind()` from a classifier and a regressor, before either is
trained.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> classifier =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> regressor =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});

  std::cout << "classifier task_kind == Classification: " << std::boolalpha
            << (classifier.value()->task_kind() == cls::TaskKind::Classification) << "\n";
  std::cout << "regressor task_kind == Regression: "
            << (regressor.value()->task_kind() == cls::TaskKind::Regression) << "\n";

  return 0;
}
```

Output

```text
classifier task_kind == Classification: true
regressor task_kind == Regression: true
```

### Example 2

`task_kind()` does not change across `learn` — it reflects the estimator's
fixed type, not anything learned from data.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::StatusOr<std::unique_ptr<cls::RegressorBase>> made =
      cls::make_regressor(cls::ModelSpec{cls::TaskKind::Regression, cls::ModelFamily::Linear,
                                         cls::LinearFamilyOptions{}});
  const cls::TaskKind before = made.value()->task_kind();

  cls::LabeledBatch batch{};
  batch.features.x = pluss::make_series({1.0, 2.0}, {2}, f64).reshape({2, 1});
  batch.y = pluss::make_series({2.0, 4.0}, {2}, f64);
  made.value()->learn(batch);
  const cls::TaskKind after = made.value()->task_kind();

  std::cout << "same before and after learn = " << std::boolalpha << (before == after) << "\n";

  return 0;
}
```

Output

```text
same before and after learn = true
```

### Example 3

Using `task_kind()` to branch generically over an `EstimatorBase`-family
pointer without knowing the concrete family ahead of time.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  auto describe = [](cls::TaskKind kind) {
    switch (kind) {
      case cls::TaskKind::Classification: return "classification";
      case cls::TaskKind::Regression: return "regression";
      case cls::TaskKind::Transform: return "transform";
    }
    return "unknown";
  };

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Hinge;
  cls::StatusOr<std::unique_ptr<cls::ClassifierBase>> made =
      cls::make_classifier(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});

  std::cout << "this estimator solves a " << describe(made.value()->task_kind()) << " problem\n";

  return 0;
}
```

Output

```text
this estimator solves a classification problem
```

## See Also

- [`ClassifierBase`](../ClassifierBase.md) / [`RegressorBase`](../RegressorBase.md) —
  which `task_kind()` each always returns.
- [`trained`](trained.md) — the other fixed-vs-mutable state query.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_state.html">
    <span class="label">Previous</span>
    <span class="title">load_state</span>
  </a>
  <a class="next-link" href="trained.html">
    <span class="label">Next</span>
    <span class="title">trained</span>
  </a>
</div>
<!-- apiref:nav:end -->
