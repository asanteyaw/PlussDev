<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0bf2866b73b1 -->

# `lymph::workflow::Workflow::on`

[Back to `lymph::workflow::Workflow`](../Workflow.md)

`on` opens one **branch** — a named group of columns matched by a route —
and returns an accessor you chain `.add(stage)` calls onto to build that
branch's own ordered chain of prep stages. Two branches on the same
`Workflow`, each with its own chain, is the `ColumnTransformer`-shaped part
of this family: see
[Introduction's "Compared To A Pipeline And ColumnTransformer"](Introduction.md#compared-to-a-pipeline-and-columntransformer)
for the full mapping (including the flat, no-column-split case and custom
transformers) before diving into the overloads below.

## Signature

```cpp
WorkflowRoute Workflow::on(Route router);
WorkflowRoute Workflow::on(std::vector<std::string> names);
WorkflowRoute Workflow::on(std::initializer_list<std::string> names);
WorkflowRoute Workflow::on(std::string name, Route router);
WorkflowRoute Workflow::on(std::string name, std::vector<std::string> names);
WorkflowRoute Workflow::on(std::string name, std::initializer_list<std::string> names);
WorkflowRoute Workflow::on(
    std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate);
WorkflowRoute Workflow::on(
    std::string name,
    std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate);

WorkflowBranch Workflow::branch(std::string name);
WorkflowRoute WorkflowBranch::route(Route router);
WorkflowRoute WorkflowBranch::route(std::vector<std::string> names);
WorkflowRoute WorkflowBranch::route(std::initializer_list<std::string> names);
WorkflowRoute WorkflowBranch::route(
    std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate);

WorkflowRoute& WorkflowRoute::add(Stage stage);
```

## What It Does

`on` starts a branch on the workflow. It names a group of columns through a route and returns an accessor that adds stages to that branch's own ordered chain. Several branches on one workflow give each group its own transforms. A route can be a list of names, a predicate over feature names and dtypes, or a route object.

## Parameters

`std::string name`
: The branch's label. Present only on the named overloads; it is pure
  bookkeeping (shows up in [`branches()`](../Workflow.md#state) /
  [`WorkflowBranchInfo::name`](../Workflow.md#state)) and never affects which
  columns match — leave it off (the unnamed overloads) when a branch's
  identity does not matter to the caller.

`std::vector<std::string> names` / `std::initializer_list<std::string> names`
: The exact column names this branch claims, in
  [`prep::route::names(...)`](../../prep/route/Introduction.md) form. A name
  absent from the frame at [`learn`](learn.md)/[`map`](map.md)/[`infer`](infer.md)
  time is simply not matched — `on(...)` itself never validates against a
  frame, since no frame exists yet at call time.

`std::function<bool(std::string_view feature_name, std::string_view dtype)> predicate`
: A [`prep::route::where(...)`](../../prep/route/Introduction.md) predicate,
  evaluated per column against its name and its `dtype` string (e.g.
  `"float64"`) at prep time. Use this instead of an explicit name list when a
  branch should keep matching new columns of a given shape as a frame's
  schema grows (Example 3).

`Route router`
: A pre-built [`prep::Route`](../../prep/route/Introduction.md) — the escape
  hatch for a selection strategy `names`/`where` alone cannot express, such as
  [`route::remaining_of(...)`](../../prep/route/Introduction.md).

`Stage stage`
: The prep stage to append to the branch, e.g.
  [`prep::stage::standard_scaler()`](../../prep/stage/Introduction.md) or
  [`prep::stage::one_hot_encoder()`](../../prep/stage/Introduction.md). Each
  `.add(...)` call appends one more stage onto this same branch's chain; call
  `on(...)`/`branch(...)` again to open a different branch.

## Returns

`Workflow::on(...)` and `WorkflowBranch::route(...)` both return a
`WorkflowRoute` — an accessor bound to the branch that was just opened, not a
standalone value. `WorkflowRoute::add(stage)` returns the same `WorkflowRoute`
by reference so calls chain (`.add(a).add(b)`), and every `.add(...)` call
mutates the `Workflow` that opened the branch: `on(...)` has no separate
`_`/value-returning pair the way `learn`/`map`/`infer` do, since routing a
branch is inherently building up the one `Workflow` object in place.
`Workflow::branch(name)` returns a `WorkflowBranch` — an intermediate handle
that exists only to receive one `.route(...)` call.

## Errors

- A named overload (`on(name, ...)` or `branch(name)`) throws if `name` is
  empty.
- `WorkflowRoute::add(...)` throws if called on a default-constructed/empty
  route accessor — not reachable through the public overloads above, since
  every `on(...)`/`route(...)` call returns a properly bound `WorkflowRoute`.
- `on(...)`/`route(...)` themselves never throw for an unmatched name or an
  empty match set — an empty branch (0 stages routed, 0 columns matched at
  prep time) is simply a no-op, not an error.

## Practical Examples

These examples build the `ColumnTransformer`-shaped part of `Workflow`: each
one opens one or more branches, routes columns into them by explicit name,
by the two-call `branch(name).route(...)` spelling, or by a `(name, dtype)`
predicate, then trains and predicts through the ordinary `learn`/`infer` verb
pair.

### Example 1

The baseline shape: one anonymous branch scaling the two numeric columns,
one named branch one-hot-encoding a `Note`-typed categorical column. Every
`.add(...)` appends straight onto the held `Model`'s own prep pipeline, so
`stage_count()` on the trained `Workflow` reports both stages combined.

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

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8}, {10}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4}, {10}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const DataFrame frame({{"income", Slate(income)}, {"balance", Slate(balance)},
                         {"category", Slate(pluss::make_note(std::vector<std::string>{
                             "a", "b", "a", "b", "a", "b", "a", "b", "a", "b"}))},
                         {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance", "category"};
  data_options.label_feature = "target";
  data_options.note_policy = lymph::DataNotePolicy::Factorize;

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.on({"income", "balance"}).add(lymph::prep::stage::standard_scaler());
  wf.on("category", {"category"}).add(lymph::prep::stage::one_hot_encoder());

  const workflow::Workflow trained = wf.learn(frame, data_options);
  std::cout << "branch_count = " << trained.branch_count() << "\n";
  for (const auto& b : trained.branches()) {
    std::cout << "  branch name=\"" << b.name << "\" named=" << std::boolalpha << b.named
              << " stage_count=" << b.stage_count << "\n";
  }
  std::cout << "stage_count = " << trained.stage_count() << "\n";

  const pluss::Series pred = trained.infer(frame, data_options);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
```

Output

```text
branch_count = 2
  branch name="" named=false stage_count=1
  branch name="category" named=true stage_count=1
stage_count = 2
pred = pluss::Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[0, 0, 0, 1, 1, 1, 1, 1, 0, 1]
```

### Example 2

`on(name, names)` and the two-call `branch(name).route(names)` spelling build
the identical branch — this example trains one `Workflow` each way on the
same frame and shows the predictions come out bit-for-bit the same. Reach for
`branch(name)` when a route is decided somewhere other than the `on(...)`
call site (e.g. built up conditionally before it is attached).

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

  const pluss::Series income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8}, {10}, f64);
  const pluss::Series balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4}, {10}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const DataFrame frame({{"income", Slate(income)}, {"balance", Slate(balance)},
                         {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"income", "balance"};
  data_options.label_feature = "target";

  workflow::Workflow one_call = workflow::make(lymph::models::logistic_regression());
  one_call.on("numeric", {"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  workflow::Workflow two_call = workflow::make(lymph::models::logistic_regression());
  two_call.branch("numeric").route({"income", "balance"}).add(lymph::prep::stage::standard_scaler());

  const workflow::Workflow trained_one = one_call.learn(frame, data_options);
  const workflow::Workflow trained_two = two_call.learn(frame, data_options);

  const pluss::Series pred_one = trained_one.infer(frame, data_options);
  const pluss::Series pred_two = trained_two.infer(frame, data_options);

  std::cout << "one_call branch name=\"" << trained_one.branches().front().name
            << "\" stage_count=" << trained_one.branches().front().stage_count << "\n";
  std::cout << "two_call branch name=\"" << trained_two.branches().front().name
            << "\" stage_count=" << trained_two.branches().front().stage_count << "\n";
  std::cout << "pred_one = " << pred_one << "\n";
  std::cout << "pred_two = " << pred_two << "\n";

  return 0;
}
```

Output

```text
one_call branch name="numeric" stage_count=1
two_call branch name="numeric" stage_count=1
pred_one = pluss::Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[0, 0, 0, 1, 1, 1, 1, 0, 0, 1]
pred_two = pluss::Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[0, 0, 0, 1, 1, 1, 1, 0, 0, 1]
```

### Example 3

A predicate route matches by `(name, dtype)` instead of an explicit list, so
it keeps matching every `"num_*"` column as a frame's schema grows without
the `Workflow` definition itself changing — useful when the exact feature set
is assembled upstream and only follows a naming convention.

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

  const pluss::Series num_income = pluss::make_series(
      {1.0, 1.4, 1.8, 2.2, 2.6, 3.0, 2.4, 1.6, 2.0, 2.8}, {10}, f64);
  const pluss::Series num_balance = pluss::make_series(
      {0.6, 1.0, 1.3, 1.6, 1.9, 2.2, 2.6, 2.0, 0.8, 1.4}, {10}, f64);
  const pluss::Series num_tenure = pluss::make_series(
      {1.0, 2.0, 1.0, 3.0, 2.0, 4.0, 3.0, 1.0, 2.0, 3.0}, {10}, f64);
  const pluss::Series y = pluss::make_series(
      {0, 0, 0, 0, 0, 1, 1, 1, 1, 1}, {10}, f64);
  const DataFrame frame({{"num_income", Slate(num_income)},
                         {"num_balance", Slate(num_balance)},
                         {"num_tenure", Slate(num_tenure)},
                         {"target", Slate(y)}});

  lymph::options::DFBatch data_options{};
  data_options.feature_names = {"num_income", "num_balance", "num_tenure"};
  data_options.label_feature = "target";

  workflow::Workflow wf = workflow::make(lymph::models::logistic_regression());
  wf.on([](std::string_view name, std::string_view) { return name.rfind("num_", 0) == 0; })
      .add(lymph::prep::stage::standard_scaler());

  const workflow::Workflow trained = wf.learn(frame, data_options);
  std::cout << "branch_count = " << trained.branch_count() << "\n";
  const auto& b = trained.branches().front();
  std::cout << "branch name=\"" << b.name << "\" named=" << std::boolalpha << b.named
            << " stage_count=" << b.stage_count << "\n";

  const pluss::Series pred = trained.infer(frame, data_options);
  std::cout << "pred = " << pred << "\n";

  return 0;
}
```

Output

```text
branch_count = 1
branch name="" named=false stage_count=1
pred = pluss::Series(shape=[10], strides=[1], dtype=float64, platform=cpu, numel=10, contiguous=true)
data=[0, 0, 0, 1, 1, 1, 1, 0, 0, 1]
```

## See Also

- [`learn`](learn.md) — fits the `Model` this branch DSL has been routing
  stages onto.
- [`with`](with.md) — replays every branch/route step recorded through
  `on(...)`/`branch(...)` onto a different base model.
- [`prep::Route`](../../prep/route/Introduction.md) — the underlying
  `names`/`where`/`remaining_of` selection primitive.
- [`prep::stage`](../../prep/stage/Introduction.md) — the stages `.add(...)`
  accepts.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="save.html">
    <span class="label">Next</span>
    <span class="title">save</span>
  </a>
</div>
<!-- apiref:nav:end -->
