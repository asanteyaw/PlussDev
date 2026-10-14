<!-- pluss-hand-authored -->

# Locus::materialize

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::materialize(const JoinPlan& plan, const JoinOptions& options = {}) const -> Catalog
```

## What It Does

Runs a `JoinPlan` -- one native [`join`](join.md) per step, in order,
accumulating columns table by table, each hop enforcing its own link's
cardinality the same as a direct `join()` call would. This is the method
[`run`](run.md) calls internally once a plan is resolved, whether that plan
came from [`plan()`](plan.md) directly or from a [saved plan](store.md)
looked up by name.

## Parameters

| Parameter | Meaning |
|---|---|
| `plan` | The `JoinPlan` to execute. |
| `options` | Raggedness policy applied to every hop. |

## Returns

A `Catalog`: the result of joining every step of the plan.

## Examples

Example 1. Materializing a plan built by hand.

```cpp
JoinPlan plan;
plan.root_table = "orders";
plan.result_name = "orders_with_customers";
JoinPlanStep step;
step.left_table = "orders";
step.right_table = "customers";
step.spec.keys = {{"cust_id", "id"}};
step.spec.how = JoinHow::Inner;
plan.steps.push_back(step);
const auto linked = make_linked_shop();
const auto direct = linked.materialize(plan);
std::cout << "materialize rows=" << direct.get("amount")->size() << "\n";
```

### Output

```text
materialize rows=3
```

The default `Inner` join at this one step drops order `13` (customer `9`
doesn't exist), matching [`join`](join.md)'s own single-hop behavior.

## See Also

- [`plan`](plan.md): building the `JoinPlan` this executes.
- [`run`](run.md): the saved-plan-by-name convenience this backs.
- [`join`](join.md): the single-hop operation each step performs.
