<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8ed987744539 -->

# `pluss::ai::classic::Flow::nested_param_keys`

`nested_param_keys` lists every path string [`nested_param`](nested_param.md) /
[`set_nested_param`](set_nested_param.md) accept on this specific `Flow` —
one entry per stage's route metadata and configuration field, plus the
attached model's own hyperparameters if one is set. It exists so generic
tooling (a hyperparameter search, a config dumper) can discover what is
tunable on an arbitrary `Flow` without hardcoding stage or family knowledge.

## Signature

```cpp
[[nodiscard]] std::vector<std::string> classic::Flow::nested_param_keys() const;
```

## Parameters

None.

## Returns

A `std::vector<std::string>`, each formatted `"<stage>::<field>"` (or
`"model::<field>"` for the attached model, if any) — `<stage>` is the
stage's key (from [`set_stage_key`](set_stage_key.md)) or the positional
fallback `"stage<i>"` if none was set. Every entry is a valid `path` for
[`nested_param`](nested_param.md) / [`set_nested_param`](set_nested_param.md)
on this exact `Flow`, in its current shape — the list changes if stages are
added, removed, or renamed afterward.

Four keys are always present per stage (`route_size`, `remainder`, `kind`,
`stage_index`), plus one `opt.<field>` key per numeric field the stage's
`TransformOptions` variant actually holds.

## Errors

None — this only reads existing structure.

## Practical Examples

### Example 1

The keys for a two-stage classification flow — one scaler, one model —
showing the per-stage and `model::*` groups.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::LinearFamilyOptions linear{};
  linear.objective = cls::LinearObjective::Logistic;
  cls::StatusOr<cls::Flow> made = cls::make_flow(
      cls::ModelSpec{cls::TaskKind::Classification, cls::ModelFamily::Linear, linear});
  cls::Flow flow = made.move_value();
  flow.add_standard_scaler();
  flow.set_stage_key(0, "scale");

  for (const std::string& key : flow.nested_param_keys()) {
    std::cout << key << "\n";
  }

  return 0;
}
```

Output

```text
scale::route_size
scale::remainder
scale::kind
scale::stage_index
scale::opt.with_centering
scale::opt.with_scaling
scale::opt.variance_floor
model::task
model::family
model::opt.objective
model::opt.regularization
model::opt.regularization_strength
model::opt.elastic_l1_ratio
model::opt.robust_transition
model::opt.fit_intercept
```

### Example 2

A transform-only `Flow` (no model) has no `model::*` keys at all.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler();
  for (const std::string& key : flow.nested_param_keys()) {
    std::cout << key << "\n";
  }

  return 0;
}
```

Output

```text
stage0::route_size
stage0::remainder
stage0::kind
stage0::stage_index
stage0::opt.with_centering
stage0::opt.with_scaling
stage0::opt.variance_floor
```

### Example 3

Adding a second stage grows the key list — each stage's own positional
fallback name (`stage<i>`), since neither was given an explicit key here.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ml.h>

#include <iostream>

int main() {
  namespace cls = pluss::ai::classic;

  cls::Flow flow;
  flow.add_standard_scaler();
  std::cout << "keys with 1 stage: " << flow.nested_param_keys().size() << "\n";

  flow.add_one_hot_encoder();
  std::cout << "keys with 2 stages: " << flow.nested_param_keys().size() << "\n";
  for (const std::string& key : flow.nested_param_keys()) {
    std::cout << "  " << key << "\n";
  }

  return 0;
}
```

Output

```text
keys with 1 stage: 7
keys with 2 stages: 14
  stage0::route_size
  stage0::remainder
  stage0::kind
  stage0::stage_index
  stage0::opt.with_centering
  stage0::opt.with_scaling
  stage0::opt.variance_floor
  stage1::route_size
  stage1::remainder
  stage1::kind
  stage1::stage_index
  stage1::opt.max_categories
  stage1::opt.overflow
  stage1::opt.include_unknown_bucket
```

## See Also

- [`nested_param`](nested_param.md) / [`set_nested_param`](set_nested_param.md) —
  read/write one key from this list.
- [`set_stage_key`](set_stage_key.md) — naming a stage so its keys read
  `"<name>::..."` instead of `"stage<i>::..."`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nested_param.html">
    <span class="label">Previous</span>
    <span class="title">nested_param</span>
  </a>
  <a class="next-link" href="save_blueprint.html">
    <span class="label">Next</span>
    <span class="title">save_blueprint</span>
  </a>
</div>
<!-- apiref:nav:end -->
