<!-- pluss-hand-authored -->

# Nexus::store

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
Nexus::store(std::string_view name) const -> std::optional<JoinPlan>

Nexus::store(std::string_view name, const JoinPlan& plan) const -> Nexus
Nexus& Nexus::store_(std::string_view name, const JoinPlan& plan)
```

## What It Does

Three overloads covering both directions: the one-argument overload **reads back** a saved
[`JoinPlan`](plan.md) by name (a getter, following [`table_meta()`](table_meta.md)'s
pattern of returning `std::nullopt` for an unknown name rather than throwing); the
two-argument overloads **save** a `JoinPlan` under a name, in this class's usual
non-mutating/mutating pairing.

Saving normalizes the plan first: every step's `left_table` must chain correctly from the
previous step's `right_table` (or from `plan.root_table` for the first step), every table
named must exist in the workspace, and any step left without an explicit
`right_suffix`/`relationship_name` gets one filled in the same way
[`Nexus::join()`](join.md) would. This means a saved view, once retrieved via
[`run()`](run.md), behaves identically to calling [`materialize()`](materialize.md) on the
plan directly — saving doesn't change what the plan *does*, only that it now has a
retrievable name.

## Parameters

**name** (`std::string_view`)
    The view name. For the saving overloads: must be non-empty.

**plan** (`const JoinPlan&`, saving overloads)
    The plan to save. `plan.root_table` must be non-empty and must exist in the workspace;
    each step's tables must exist and chain correctly (see above).

## Returns

For the one-argument overload: `std::optional<JoinPlan>`
    The saved plan (after normalization), or `std::nullopt` if `name` isn't a saved view.

For the two-argument overloads: `Nexus` / `Nexus&`
    A new workspace (or, for `store_`, the same mutated workspace) with the view saved.

## Examples

Example 1. Save a plan, then read it back.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Slate(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 2.0}))},
});

auto biz = Nexus({
    {"customers", customers},
    {"orders", orders},
});

Relationship rel;
rel.name = "orders_to_customers";
rel.left_table = "orders";
rel.right_table = "customers";
rel.keys = {{"customer_id", "customer_id"}};

auto linked = biz.add_relationship(rel);

auto plan_value = linked.plan("orders", "customers", JoinHow::Left);
auto stored = linked.store("orders_with_customers", plan_value);

auto fetched = stored.store("orders_with_customers");
std::cout << std::boolalpha << fetched.has_value() << " " << fetched->root_table << "\n";

auto missing = stored.store("nope");
std::cout << std::boolalpha << missing.has_value();
```

### Output

```text
true orders
false
```

Example 2. `store_` mutates the current handle in place.

```cpp
auto mutable_biz = linked;
mutable_biz.store_("orders_with_customers", plan_value);

std::cout << std::boolalpha << linked.has_store("orders_with_customers") << " "
          << mutable_biz.has_store("orders_with_customers");
```

### Output

```text
false true
```

## Notes

- The one-argument overload never throws — an unknown `name` returns `std::nullopt`.
- Saving normalizes the plan (right suffixes, relationship auto-detection) up front; a
  saved-and-retrieved plan runs identically to the original via
  [`materialize()`](materialize.md).
- [`Nexus::run()`](run.md) is the usual way to actually execute a saved view, rather than
  fetching it back with this method and calling [`materialize()`](materialize.md) yourself
  — though both approaches produce the same result.

## See Also

- [`Nexus::has_store`](has_store.md): check whether a view exists without unwrapping the
  `std::optional`.
- [`Nexus::run`](run.md): execute a saved view by name.
- [`Nexus::plan`](plan.md) / [`materialize`](materialize.md): build and execute the
  `JoinPlan` this method saves.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="summary.html">
    <span class="label">Next</span>
    <span class="title">summary</span>
  </a>
</div>
