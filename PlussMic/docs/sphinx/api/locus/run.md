<!-- pluss-hand-authored -->

# Locus::run

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Catalog Locus::run(std::string_view name, const JoinOptions& options = {}) const
Catalog Locus::run(std::string_view name, const JoinPlan& plan, const JoinOptions& options = {}) const
```

## What It Does

Runs a plan by name through [`materialize`](materialize.md). The
one-plan-argument overload looks `name` up among [saved plans](store.md)
and throws if none exists; the three-argument overload runs `plan` directly
without requiring it to already be saved under `name` (an "anonymous" run).
A saved plan follows [`rename`](rename.md) the same way a declared link
does, so a renamed catalog's saved plan still runs correctly afterward.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | A saved plan's name to look up, or a label for an inline run. |
| `plan` | The `JoinPlan` to run directly, for the three-argument overload. |
| `options` | Raggedness policy applied to every hop. |

## Returns

A `Catalog`: the plan's materialized result.

## Errors

The two-argument overload throws when no plan is saved under `name`.

## Examples

Example 1. Running a saved plan, and running one inline without saving it
first.

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
std::cout << "run rows=" << stored.run("by_customer").get("amount")->size() << "\n";
std::cout << "run with inline plan rows=" << linked.run("anonymous", plan).get("amount")->size() << "\n";
```

### Output

```text
run rows=3
run with inline plan rows=3
```

Example 2. An unknown saved plan name is refused; a renamed catalog's saved
plan still runs.

```cpp
print_error("run unknown", [&] { (void)linked.run("missing"); });
const auto renamed_store = stored.rename("customers", "people");
std::cout << "renamed plan still runs rows=" << renamed_store.run("by_customer").get("amount")->size() << "\n";
```

### Output

```text
run unknown: Locus::run: unknown saved plan 'missing'
renamed plan still runs rows=3
```

## See Also

- [`store`](store.md): saving the plan `run` looks up by name.
- [`materialize`](materialize.md): the lower-level method `run` calls.
