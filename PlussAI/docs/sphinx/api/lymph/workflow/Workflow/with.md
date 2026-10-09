<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a6e347ba478b -->

# `lymph::workflow::Workflow::with`

[Back to `lymph::workflow::Workflow`](../Workflow.md)

## Signature

```cpp
Workflow Workflow::with(Model model) const;
```

## What It Does

`with` returns a new `Workflow` on a **different base `Model`**, but keeps
this `Workflow`'s `Policy`, branches, and every step recorded through
[`on`](on.md)/`branch(...)` — it **replays** those recorded steps onto
`model` rather than copying this `Workflow`'s already-materialized state
(which would carry over whatever this `Workflow`'s own base model had fitted,
if anything). The receiver is left untouched, the same value semantics
`learn`/`learn_` follow (Example 3).

Reach for it when a route/`Policy` design is decided once and several
candidate models need to reuse it without re-typing the routing — a
`search`/`spotcheck`-style factory closure is the typical caller. A
`Workflow` built with no model at all
(`workflow::make(policy)` — see [`make`](../make.md)) is the natural starting
point for a route-first design that only picks its model at the very end
(Example 1).

## Parameters

`model`
: The new base `Model` — typically a fresh factory template
  (`models::logistic_regression()`, `models::tree::decision_tree_classifier()`,
  …), though any `Model` value works. Its own prep stages (if it has any from
  `prepare()`/`append(...)`) stay attached underneath the replayed steps.

## Returns

A new `Workflow` holding `model`, with the same `Policy`, `branches()`, and
`stage_count()`-worth of routed steps this `Workflow` had recorded — none of
it materialized/fitted yet, since `model` starts as whatever training state
it was passed in with (usually untrained).

## Errors

None beyond what constructing the replayed `Workflow` itself could raise
(an empty named branch, which cannot occur here since branch names were
already validated when originally recorded).

## Practical Examples

### Example 1

A route-first "design" `Workflow` — built with `workflow::make(policy)`, no
model at all — picks its model only at the end via `with(...)`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  workflow::Workflow design = workflow::make(workflow::Policy{});
  design.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());
  std::cout << "design.has_model() = " << std::boolalpha << design.has_model() << "\n";

  const workflow::Workflow candidate = design.with(lymph::models::logistic_regression());
  std::cout << "candidate.has_model() = " << candidate.has_model()
            << "  candidate.branch_count() = " << candidate.branch_count() << "\n";

  const workflow::Workflow trained = candidate.learn(raw, data_options);
  std::cout << "trained.stage_count() = " << trained.stage_count() << "\n";
  std::cout << "pred = " << trained.infer(raw, data_options) << "\n";

  return 0;
}
```

Output

```text
design.has_model() = false
candidate.has_model() = true  candidate.branch_count() = 1
trained.stage_count() = 1
pred = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 2

Swapping the base model on an already-model-bearing `Workflow` — the same
routed prep trains through both a logistic regression and a decision tree.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  workflow::Workflow logistic_wf = workflow::make(lymph::models::logistic_regression());
  logistic_wf.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  const workflow::Workflow tree_wf = logistic_wf.with(lymph::models::tree::decision_tree_classifier());
  std::cout << "tree_wf.branch_count() = " << tree_wf.branch_count() << "\n";

  const workflow::Workflow trained_logistic = logistic_wf.learn(raw, data_options);
  const workflow::Workflow trained_tree = tree_wf.learn(raw, data_options);
  std::cout << "logistic task_kind == tree task_kind: "
            << std::boolalpha << (trained_logistic.task_kind() == trained_tree.task_kind()) << "\n";

  return 0;
}
```

Output

```text
tree_wf.branch_count() = 1
logistic task_kind == tree task_kind: true
```

### Example 3

`with` never mutates the receiver: `design` stays trained-or-not exactly as
it was, independent of what happens to `candidate` afterward.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Slate;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Slate(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Slate(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Slate(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  workflow::Workflow design = workflow::make(lymph::models::logistic_regression());
  design.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());
  design.learn_(raw, data_options);
  std::cout << "design.trained() before with(...) = " << std::boolalpha << design.trained() << "\n";

  const workflow::Workflow candidate = design.with(lymph::models::logistic_regression());
  std::cout << "design.trained() after with(...)   = " << design.trained() << "  (unchanged)\n";
  std::cout << "candidate.trained()                = " << candidate.trained() << "  (fresh base model)\n";

  const workflow::Workflow retrained = candidate.learn(raw, data_options);
  std::cout << "retrained.trained()                = " << retrained.trained() << "\n";

  return 0;
}
```

Output

```text
design.trained() before with(...) = true
design.trained() after with(...)   = true  (unchanged)
candidate.trained()                = false  (fresh base model)
retrained.trained()                = true
```

## See Also

- [`on`](on.md) — records the branch/route steps `with` replays.
- [`make`](../make.md) — `workflow::make(policy)`, the model-free starting
  point for a route-first design.
- [`learn`](learn.md) — the same by-value/receiver-untouched semantics.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save.html">
    <span class="label">Previous</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
