<!-- pluss-hand-authored -->

# Nexus::orphans

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::orphans(std::string_view relationship_name, RelationshipSide side = RelationshipSide::Left) const -> DataFrame
```

## What It Does

An exact alias for [`detect_orphans()`](detect_orphans.md) — `orphans` calls straight
through with no behavioral difference. `orphans` is the shorter, more conversational
spelling.

## Parameters

See [`detect_orphans()`](detect_orphans.md) — identical here.

## Returns

Identical to [`detect_orphans()`](detect_orphans.md).

## Examples

Example 1. `orphans()` and `detect_orphans()` return identical results.

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben"}))},
});
auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 4.0}))},
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

std::cout << linked.orphans("orders_to_customers").ncases();
```

### Output

```text
1
```

## Notes

- Purely a naming alternative — see [`detect_orphans()`](detect_orphans.md) for the full
  documentation, including the meaning of `RelationshipSide`.

## See Also

- [`Nexus::detect_orphans`](detect_orphans.md): the canonical name this aliases.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ntables.html">
    <span class="label">Previous</span>
    <span class="title">ntables</span>
  </a>
  <a class="next-link" href="path.html">
    <span class="label">Next</span>
    <span class="title">path</span>
  </a>
</div>
