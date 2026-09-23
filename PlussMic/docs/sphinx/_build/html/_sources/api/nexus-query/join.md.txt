<!-- pluss-hand-authored -->

# NexusTraverse::join

[Back to NexusTraverse API Reference](../../nexus_traverse_api_reference)

## Signatures

```cpp
NexusTraverse::join(std::string_view right) const -> NexusTraverse
NexusTraverse::join(std::string_view right, const JoinSpec& spec) const -> NexusTraverse
```

## What It Does

Extends the query by one hop, joining the query's current anchor table (initially whatever
you passed to [`Nexus::from()`](../nexus/from.md), then whichever table the most recent
`.join()`/`.via()` call landed on) to `right`. Each call appends one step to the
accumulated `JoinPlan` and moves the anchor forward to `right`, so a chain of `.join()`
calls builds up a multi-hop plan without you constructing
[`JoinPathStep`](../nexus/path.md)s by hand.

```{important} The single-argument overload auto-detects the relationship — and defaults to Inner
Unlike [`Nexus::join()`](../nexus/join.md), which always requires an explicit `keys` list
or `relationship_name` in its `JoinSpec`, `NexusTraverse::join(right)` (no `spec`) looks for
exactly one relationship directly connecting the current anchor and `right`, using it
automatically. If more than one relationship connects the same pair, the call throws,
telling you to disambiguate via `JoinSpec::relationship_name`. Also note the resolved
`JoinSpec`'s `how` defaults to `JoinHow::Inner` (matching `JoinSpec`'s own default) —
different from [`.via()`](via.md), which defaults to `JoinHow::Left`. If you want a
left join through `.join()`, set `spec.how` explicitly.
```

## Parameters

**right** (`std::string_view`)
    The table to join in next. Must already exist in the workspace.

**spec** (`const JoinSpec&`, two-argument overload)
    Same struct [`Nexus::join()`](../nexus/join.md) takes. If `spec.relationship_name` and
    `spec.keys` are both left unset (and `spec.how` isn't `Cross`), the relationship is
    auto-detected the same way the one-argument overload does. `spec.right_suffix`, if left
    empty, defaults to `"_" + right`.

## Returns

`NexusTraverse`
    A new query with the join step appended and the anchor moved to `right`. The original
    query is unchanged — like the rest of this class's non-mutating, chainable design.

## Examples

Example 1. A single auto-detected hop.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 2.0}))},
    {"amount",      Thread(pluss::make_series({50.0, 75.0, 20.0}))},
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

auto result = linked.from("orders").join("customers").collect();
std::cout << result.ncases();
```

### Output

```text
3
```

Example 2. Chaining two `.join()` calls defaults to `Inner` at each hop — contrast
[`.via()`](via.md)'s `Left` default, which would keep every `customers` row even without
a matching order.

```cpp
auto payments = DataFrame({
    {"payment_id", Thread(pluss::make_series({9001.0, 9002.0}))},
    {"order_id",   Thread(pluss::make_series({101.0, 102.0}))},
});
auto with_payments = linked.inject("payments", payments);

Relationship payments_to_orders;
payments_to_orders.name = "payments_to_orders";
payments_to_orders.left_table = "payments";
payments_to_orders.right_table = "orders";
payments_to_orders.keys = {{"order_id", "order_id"}};
auto fully_linked = with_payments.add_relationship(payments_to_orders);

auto chained = fully_linked.from("customers").join("orders").join("payments").collect();
std::cout << chained.ncases();
```

### Output

```text
2
```

Order `103` has no payment and customer `3` has no orders at all — both inner-joined away,
leaving only the two order/payment pairs that matched at every hop.

Example 3. When more than one relationship connects the same pair, the auto-detecting
overload throws.

```cpp
Relationship second;
second.name = "orders_to_customers_2";
second.left_table = "orders";
second.right_table = "customers";
second.keys = {{"customer_id", "customer_id"}};

auto doubly_linked = linked.add_relationship(second);

try {
  auto bad = doubly_linked.from("orders").join("customers");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
NexusTraverse::join: multiple direct relationships connect 'orders' and 'customers'. Specify JoinSpec.relationship_name explicitly
```

## Notes

- The one-argument overload requires exactly one direct relationship between the current
  anchor and `right`; zero throws (no connection), more than one also throws (ambiguous) —
  disambiguate with `spec.relationship_name` in the two-argument overload.
- `spec.how` defaults to `JoinHow::Inner` when left unset — different from
  [`.via()`](via.md)'s `Left` default. See the important callout above.
- Non-mutating and chainable — each call returns a new `NexusTraverse`.

## See Also

- [`NexusTraverse::via`](via.md): reach a table by its shortest graph path (possibly several
  hops) in one call, defaulting to `Left`, instead of one explicit hop at a time.
- [`Nexus::join`](../nexus/join.md): the lower-level, non-auto-detecting primitive this
  method builds on.
- [`NexusTraverse::collect`](collect.md): execute the accumulated plan.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="via.html">
    <span class="label">Next</span>
    <span class="title">via</span>
  </a>
</div>
