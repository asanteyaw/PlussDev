<!-- pluss-hand-authored -->

# Locus::has_store, Locus::stores

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::has_store(std::string_view name) const -> bool
Locus::stores() const -> std::vector<std::string>
```

## What It Does

`has_store` reports whether a `JoinPlan` has been saved under `name` (see
[`store`](store.md)). `stores()` lists every saved plan's name, sorted.
Both are read-only; neither executes a plan.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | A saved plan's name, for `has_store`. |

## Returns

`has_store` returns `bool`. `stores` returns a sorted
`std::vector<std::string>`.

## Examples

Example 1. Before and after saving one plan.

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
std::cout << "has_store before=" << linked.has_store("by_customer") << "\n";
const auto stored = linked.store("by_customer", plan);
std::cout << "has_store after=" << stored.has_store("by_customer")
          << " original has_store=" << linked.has_store("by_customer") << "\n";
std::cout << "stores()=[";
const auto names = stored.stores();
for (std::size_t i = 0; i < names.size(); ++i) { if (i) std::cout << ", "; std::cout << names[i]; }
std::cout << "]\n";
```

### Output

```text
has_store before=false
has_store after=true original has_store=false
stores()=[by_customer]
```

`store(...)` returns a new `Locus` -- `linked` itself never gains the saved
plan.

## See Also

- [`store`](store.md): saving a plan under a name, and reading one back.
- [`run`](run.md): executing a saved plan by name.
