<!-- pluss-hand-authored -->

# Nexus::audit

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::audit(std::string_view name) const -> DataFrame
```

## What It Does

An exact alias for [`relationship_audit()`](relationship_audit.md) — `audit` calls
straight through with no behavioral difference. `audit` is the shorter, more
conversational spelling.

## Parameters

See [`relationship_audit()`](relationship_audit.md) — identical here.

## Returns

Identical to [`relationship_audit()`](relationship_audit.md).

## Examples

Example 1. `audit()` and `relationship_audit()` return identical results.

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben"}))},
});
auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
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

std::cout << linked.audit("orders_to_customers").features("valid");
```

### Output

```text
0  true
Name: valid, ptype: Series
```

## Notes

- Purely a naming alternative — see [`relationship_audit()`](relationship_audit.md) for the
  full documentation, including every returned column.

## See Also

- [`Nexus::relationship_audit`](relationship_audit.md): the canonical name this aliases.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply_all.html">
    <span class="label">Previous</span>
    <span class="title">apply_all</span>
  </a>
  <a class="next-link" href="check.html">
    <span class="label">Next</span>
    <span class="title">check</span>
  </a>
</div>
