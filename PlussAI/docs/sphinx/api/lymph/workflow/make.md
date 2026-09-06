<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 000000000000 -->

# `lymph::workflow::make`

[Back to `workflow`](../workflow.md)

## Signature

```cpp
Workflow workflow::make(Policy policy);
Workflow workflow::make(Model model, Policy policy = {});
Workflow workflow::make(
    Policy policy, std::vector<WorkflowBranchSpec> branches, WorkflowOptions options = {});
Workflow workflow::make(
    Policy policy, std::vector<WorkflowBranchSpec> branches,
    Model model, WorkflowOptions options = {});
Workflow workflow::make(
    std::vector<WorkflowBranchSpec> branches, Model model, WorkflowOptions options = {});
// Each std::vector<WorkflowBranchSpec> overload above also has an
// std::initializer_list<WorkflowBranchSpec> sibling for a braced literal at
// the call site -- same behavior, not separately exampled here.

WorkflowBranchSpec workflow::branch(
    std::string name, std::vector<std::string> columns, std::vector<WorkflowStepSpec> steps);

Workflow workflow::restore(const io::FlowBundle& bundle, Policy policy = {});
Workflow workflow::load(const std::string& path);
```

## What It Does

`make` builds a [`Workflow`](Workflow.md) — always starting either from an
empty, model-free template (`make(policy)`) or from a `Model` you already
have (every other overload). The three-or-four-argument overloads are the
**declarative** counterpart to the imperative
[`.on(...).add(stage)`](Workflow/on.md) DSL: build every branch's columns and
steps as data (`workflow::branch(...)` values) and hand them to `make` in one
call, instead of chaining `.on(...)` calls on an already-constructed
`Workflow`. Both styles produce the identical routed `Workflow` — reach for
the declarative form when a branch layout is itself assembled from
configuration rather than typed at the call site.

`restore`/`load` are the reload side: [`Workflow::save`](Workflow/save.md)
writes a model bundle plus a `Policy` sidecar file; `restore` rebuilds a
`Workflow` from an already-loaded `io::FlowBundle` and an **explicit**
`Policy` you supply; `load` is the everyday path — it reads the bundle *and*
the sidecar from `path` in one call
(`restore(resolve(io::load(path)), load_policy(path))`). Use `restore`
directly only when the `Policy` should come from somewhere other than the
sidecar file `save` wrote.

## Parameters

`policy`
: The admission contract. Every overload except the bare
  `(branches, model, options)` form takes it explicitly; that one form
  defaults it to `Policy{}` (admit everything) — see [`Policy`](Policy.md).

`model`
: The base [`Model`](../models/Model.md). Where omitted (`make(policy)`),
  the `Workflow` starts with no model at all — see
  [`Workflow::with`](Workflow/with.md) for attaching one afterward.

`branches`
: A list of `WorkflowBranchSpec` values (see `workflow::branch` below), each naming a
  branch's columns and prep steps declaratively. Applied in order, exactly
  as repeated `.on(name, columns).add(stage)` calls would be; a non-empty
  list also marks the built `Workflow` [`joined()`](Workflow.md#state).

`options`
: A [`WorkflowOptions`](Workflow.md#state) — currently only `workers` is
  applied by `make` itself (`schedule`/`cache` are read elsewhere).

`bundle`
: An already-loaded `io::FlowBundle` (from `resolve(io::load(path), ...)`)
  for `restore`.

`path`
: The bundle file path for `load` — the same path a prior
  [`Workflow::save(path, ...)`](Workflow/save.md) call wrote.

### `workflow::branch`

`WorkflowBranchSpec workflow::branch(name, columns, steps)`
: Builds one branch specification: `name` (non-empty, required by every
  `make(...)` overload that consumes it), `columns` (the exact names this
  branch claims), `steps` — a list of `WorkflowStepSpec{name, stage}`, each
  also requiring a non-empty `name` (used only as a label; it does not need
  to match anything).

## Returns

A `Workflow` value — untrained unless `model` was already trained (unusual,
but not rejected).

## Errors

- A branch with an empty `name`, or a step with an empty `name`, throws when
  `make(...)` applies it (mirrors [`on`](Workflow/on.md#errors)'s empty-name
  rejection).
- A branch with no `columns` throws.
- `restore`/`load` propagate whatever `io::load`/`Model::restore` themselves
  raise for a malformed or missing bundle file.

## Practical Examples

### Example 1

`make(policy)` alone: a model-free template, `Policy` attached up front.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  const workflow::Workflow wf = workflow::make(policy);
  std::cout << "has_model = " << std::boolalpha << wf.has_model()
            << "  policy.drop_columns[0] = " << wf.policy().drop_columns.front() << "\n";

  return 0;
}
```

Output

```text
has_model = false  policy.drop_columns[0] = customer_id
```

### Example 2

`make(model, policy)` — the everyday shape; `policy` defaults to `Policy{}`
when omitted.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  workflow::Policy policy{};
  policy.drop({"customer_id"});

  const workflow::Workflow wf = workflow::make(lymph::models::logistic_regression(), policy);
  std::cout << "has_model = " << std::boolalpha << wf.has_model()
            << "  trained = " << wf.trained() << "\n";

  const workflow::Workflow bare = workflow::make(lymph::models::logistic_regression());
  std::cout << "bare.policy().empty() = " << bare.policy().empty() << "\n";

  return 0;
}
```

Output

```text
has_model = true  trained = false
bare.policy().empty() = true
```

### Example 3

The declarative branch-spec DSL: every branch built in one `make(...)` call
from `workflow::branch(...)` values, instead of `.on(...).add(...)` chains.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const std::vector<workflow::WorkflowBranchSpec> branches = {
      workflow::branch("numeric", {"income", "balance"},
                       {workflow::WorkflowStepSpec{"scale", lymph::prep::stage::standard_scaler()}}),
  };

  const workflow::Workflow wf = workflow::make(workflow::Policy{}, branches, lymph::models::logistic_regression());
  std::cout << "branch_count = " << wf.branch_count() << "  joined = " << std::boolalpha << wf.joined() << "\n";

  const workflow::Workflow trained = wf.learn(raw, data_options);
  std::cout << "stage_count = " << trained.stage_count() << "\n";
  std::cout << "pred = " << trained.infer(raw, data_options) << "\n";

  return 0;
}
```

Output

```text
branch_count = 1  joined = true
stage_count = 1
pred = pluss::Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 0, 0, 1, 1, 1, 1]
```

### Example 4

`make(branches, model, options)` — `policy` defaults to `Policy{}` when a
caller only needs the branch DSL plus worker count.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  const std::vector<workflow::WorkflowBranchSpec> branches = {
      workflow::branch("numeric", {"income", "balance"},
                       {workflow::WorkflowStepSpec{"scale", lymph::prep::stage::standard_scaler()}}),
  };

  workflow::WorkflowOptions options{};
  options.workers = 2;

  const workflow::Workflow wf = workflow::make(branches, lymph::models::logistic_regression(), options);
  std::cout << "policy().empty() = " << std::boolalpha << wf.policy().empty()
            << "  workers = " << wf.workers()
            << "  branch_count = " << wf.branch_count() << "\n";

  return 0;
}
```

Output

```text
policy().empty() = true  workers = 2  branch_count = 1
```

### Example 5

`workflow::branch(...)` in isolation — the value every declarative
`make(...)` overload above consumes.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  const workflow::WorkflowBranchSpec spec = workflow::branch(
      "numeric", {"income", "balance"},
      {workflow::WorkflowStepSpec{"scale", lymph::prep::stage::standard_scaler()},
       workflow::WorkflowStepSpec{"impute", lymph::prep::stage::simple_imputer()}});

  std::cout << "name = " << spec.name << "  columns = " << spec.columns.size()
            << "  steps = " << spec.steps.size() << "\n";
  for (const auto& step : spec.steps) {
    std::cout << "  step name = " << step.name << "\n";
  }

  return 0;
}
```

Output

```text
name = numeric  columns = 2  steps = 2
  step name = scale
  step name = impute
```

### Example 6

`restore(bundle, policy)` — rebuilding from an already-loaded `FlowBundle`
with an explicit `Policy`, bypassing any sidecar file.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression()).learn(raw, data_options);

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_workflow_make_example_6.bundle").string();
  trained.model().save(path);
  const lymph::io::FlowBundle bundle = lymph::resolve(lymph::io::load(path), "load");

  workflow::Policy policy{};
  policy.require({"income", "balance"});
  const workflow::Workflow restored = workflow::restore(bundle, policy);
  std::cout << "restored.trained() = " << std::boolalpha << restored.trained()
            << "  policy().require_columns.size() = " << restored.policy().require_columns.size() << "\n";

  std::filesystem::remove(path);

  return 0;
}
```

Output

```text
restored.trained() = true  policy().require_columns.size() = 2
```

### Example 7

`load(path)` — the everyday reload path: model bundle and `Policy` sidecar,
read back together.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;
  using pluss::ScalarType;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const pluss::Config f64 =
      pluss::Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);

  const DataFrame raw({
      {"income", Thread(pluss::make_series({1.0, 1.3, 1.7, 2.1, 3.6, 4.0, 4.4, 4.9}, {8}, f64))},
      {"balance", Thread(pluss::make_series({0.2, 0.4, 0.5, 0.9, 1.9, 2.2, 2.5, 2.8}, {8}, f64))},
      {"default", Thread(pluss::make_series({0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64))},
  });

  workflow::Policy policy{};
  policy.require({"income", "balance", "default"});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "default";

  const workflow::Workflow trained =
      workflow::make(lymph::models::logistic_regression(), policy).learn(raw, data_options);

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_workflow_make_example_7.bundle").string();
  trained.save(path);

  const workflow::Workflow loaded = workflow::load(path);
  std::cout << "loaded.trained() = " << std::boolalpha << loaded.trained()
            << "  require_columns.size() = " << loaded.policy().require_columns.size() << "\n";

  std::filesystem::remove(path);
  std::filesystem::remove(path + ".policy");

  return 0;
}
```

Output

```text
loaded.trained() = true  require_columns.size() = 3
```

## See Also

- [`Workflow`](Workflow.md) — the type this page constructs.
- [`on`](Workflow/on.md) — the imperative, chained equivalent of the
  branch-spec DSL.
- [`save`](Workflow/save.md) — the write side `restore`/`load` read back.
- [`with`](Workflow/with.md) — attaching a model to a model-free
  `make(policy)` template later.
