<!-- pluss-hand-authored -->

# Nexus::check

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::check(std::string_view name) const -> bool
```

## What It Does

An exact alias for [`validate_relationship()`](validate_relationship.md) — `check` calls
straight through with no behavioral difference. `check` is the shorter, more
conversational spelling.

## Parameters

See [`validate_relationship()`](validate_relationship.md) — identical here.

## Returns

Identical to [`validate_relationship()`](validate_relationship.md).

## Examples

Example 1. `check()` and `validate_relationship()` return identical results.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben"}))},
});
auto orders = DataFrame({
    {"order_id",    Slate(pluss::make_series({101.0, 102.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 2.0}))},
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
rel.cardinality = RelationshipCardinality::ManyToOne;

auto linked = biz.add_relationship(rel);

std::cout << std::boolalpha << linked.check("orders_to_customers");
```

### Output

```text
true
```

Every order has a matching customer and every customer has at least one order, so this
relationship (with both sides fully covered and the required uniqueness on the right)
validates cleanly.

## Notes

- Purely a naming alternative — see [`validate_relationship()`](validate_relationship.md)
  for the full documentation, including the exact validity rules.

## See Also

- [`Nexus::validate_relationship`](validate_relationship.md): the canonical name this
  aliases.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="audit.html">
    <span class="label">Previous</span>
    <span class="title">audit</span>
  </a>
  <a class="next-link" href="deduce.html">
    <span class="label">Next</span>
    <span class="title">deduce</span>
  </a>
</div>
