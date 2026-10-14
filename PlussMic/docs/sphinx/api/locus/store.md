<!-- pluss-hand-authored -->

# Locus::store, Locus::store_

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::store(std::string_view name) const -> std::optional<JoinPlan>
Locus::store(std::string_view name, const JoinPlan& plan) const -> Locus
Locus& Locus::store_(std::string_view name, const JoinPlan& plan)
```

## What It Does

The one-argument overload reads a saved plan back by name, as
`std::nullopt` when none exists. The two-argument overload saves `plan`
under `name`, returning a new `Locus`; `store_` is the in-place form. A
saved plan's steps follow [`rename`](rename.md): renaming a catalog the
plan references updates the plan's own `JoinPlanStep::right_table`/
`left_table` fields, the same way a declared link does. A plan naming a
catalog that doesn't exist is refused at save time, not later at
[`run`](run.md) time.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | The saved plan's name. |
| `plan` | The `JoinPlan` to save, for the setter overloads. |

## Returns

The getter returns `std::optional<JoinPlan>`. The setter returns a new
`Locus` (`store`) or a reference to the mutated receiver (`store_`).

## Errors

The setter throws when `plan` names a catalog this `Locus` doesn't have.

## Examples

Example 1. Reading a saved plan back, and that it follows a rename.

```cpp
JoinPlan plan;
plan.root_table = "orders";
JoinPlanStep step;
step.left_table = "orders";
step.right_table = "customers";
step.spec.keys = {{"cust_id", "id"}};
step.spec.how = JoinHow::Inner;
plan.steps.push_back(step);

const auto linked = make_linked_shop();
const auto stored = linked.store("by_customer", plan);
std::cout << "store(name) root_table=" << stored.store("by_customer")->root_table << "\n";
const auto renamed_store = stored.rename("customers", "people");
std::cout << "renamed plan right_table=" << renamed_store.store("by_customer")->steps[0].right_table << "\n";
```

### Output

```text
store(name) root_table=orders
renamed plan right_table=people
```

Example 2. A plan naming an unknown catalog is refused.

```cpp
auto bad_plan = plan;
bad_plan.steps[0].right_table = "ghost";
print_error("store unknown catalog", [&] { (void)linked.store("bad", bad_plan); });
```

### Output

```text
store unknown catalog: Locus::store: plan names a catalog this Locus does not have
```

## See Also

- [`has_store`](has_store.md), [`stores`](has_store.md): checking what's
  saved, without reading a plan back.
- [`run`](run.md): executing a saved plan.
