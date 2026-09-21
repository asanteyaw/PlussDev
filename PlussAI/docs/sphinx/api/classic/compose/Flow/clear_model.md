<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e128295bed85 -->
# `pluss::ai::classic::Flow::clear_model`

`clear_model` removes a [`Flow`](../Flow.md)'s attached model — set by any of
[`set_model`](set_model.md), [`set_classifier`](set_classifier.md), or
[`set_regressor`](set_regressor.md) — without touching any transform stages
added independently of it. This holds for every `ModelSpec` task, including
`Transform`: see Example 3.

## Signature

```cpp
void classic::Flow::clear_model() noexcept;
```

## Parameters

None.

## Returns

`void`. Always succeeds; safe to call on a `Flow` with no model.

## Errors

None — `noexcept`, always succeeds.

## Practical Examples

### Example 1

The common case: removing a Classification/Regression model leaves transform
stages untouched.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  flow.set_model(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  std::cout << "before: has_model() = " << std::boolalpha << flow.has_model()
            << ", stage_count() = " << flow.stage_count() << "\n";

  flow.clear_model();
  std::cout << "after: has_model() = " << flow.has_model()
            << ", has_model_spec() = " << flow.has_model_spec()
            << ", stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
```

Output

```text
before: has_model() = true, stage_count() = 1
after: has_model() = false, has_model_spec() = false, stage_count() = 1
```

### Example 2

Safe on a flow with no model, and resets `trained()`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;
  const pluss::Config f64 =
      pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);

  cls::Flow flow{};
  flow.clear_model();  // no model yet -- a no-op
  std::cout << "clear_model() with no model: has_model() = " << std::boolalpha << flow.has_model() << "\n";

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  flow.set_model(cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});

  const pluss::Series x = pluss::make_series({0.0, 1.0, 2.0, 3.0}, {4}, f64).reshape({4, 1});
  const pluss::Series y = pluss::make_series({0.0, 0.0, 1.0, 1.0}, {4}, f64);
  cls::LabeledBatch batch{};
  batch.features.x = x;
  batch.y = y;
  const cls::Status learn_status = flow.learn(batch);
  std::cout << "learn ok = " << learn_status.ok() << "\n";
  std::cout << "trained() before clear_model() = " << flow.trained() << "\n";

  flow.clear_model();
  std::cout << "trained() after clear_model() = " << flow.trained() << "\n";

  return 0;
}
```

Output

```text
clear_model() with no model: has_model() = false
learn ok = true
trained() before clear_model() = true
trained() after clear_model() = false
```

### Example 3

A `Transform`-task model is removed cleanly too — `stage_count()` drops by
one, and a prep stage added before it survives.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::ClusterFamilyOptions cluster{};
  cluster.clusters = 2;
  cls::Flow flow{};
  flow.add_standard_scaler({}, {0});
  flow.set_model(cls::ModelSpec{cls::TaskKind::Transform, cls::ModelFamily::Cluster, cluster});
  std::cout << "before clear_model(): stage_count() = " << flow.stage_count()
            << ", has_model() = " << std::boolalpha << flow.has_model() << "\n";

  flow.clear_model();
  std::cout << "after clear_model(): stage_count() = " << flow.stage_count()
            << ", has_model() = " << flow.has_model() << "\n";

  // The prep stage added before the model survives; only the model was removed.
  const cls::Status s = flow.add_one_hot_encoder({}, {1});
  std::cout << "adding another stage afterward ok = " << s.ok() << "\n";
  std::cout << "final stage_count() = " << flow.stage_count() << "\n";

  return 0;
}
```

Output

```text
before clear_model(): stage_count() = 2, has_model() = true
after clear_model(): stage_count() = 1, has_model() = false
adding another stage afterward ok = true
final stage_count() = 2
```

## See Also

- [`set_model`](set_model.md) / [`set_classifier`](set_classifier.md) /
  [`set_regressor`](set_regressor.md) — what this undoes, for every task.
- [`clear_transforms`](clear_transforms.md) — clears every stage, model
  included, in one call.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assess.html">
    <span class="label">Previous</span>
    <span class="title">assess</span>
  </a>
  <a class="next-link" href="clear_transforms.html">
    <span class="label">Next</span>
    <span class="title">clear_transforms</span>
  </a>
</div>
<!-- apiref:nav:end -->
