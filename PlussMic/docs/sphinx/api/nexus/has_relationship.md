<!-- pluss-hand-authored -->

# Nexus::has_relationship

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::has_relationship(std::string_view name) const -> bool
```

## What It Does

Reports whether a relationship with the given `name` is declared in the workspace. The
safe way to check before calling [`relationship()`](relationship.md)'s value on a name you
aren't certain exists, or before methods that require a known relationship name
([`validate_relationship()`](validate_relationship.md),
[`detect_orphans()`](detect_orphans.md), [`relationship_audit()`](relationship_audit.md) —
all of which throw on an unknown name rather than returning an empty result).

## Parameters

**name** (`std::string_view`)
    The relationship name to check, matching [`Relationship::name`](relationships.md).

## Returns

`bool`
    `true` if a relationship with that exact name is declared, `false` otherwise.

## Examples

Example 1. Before and after declaring a relationship.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy"}))},
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

auto linked = biz.add_relationship(rel);

std::cout << std::boolalpha
          << linked.has_relationship("orders_to_customers") << " "
          << linked.has_relationship("nope");
```

### Output

```text
true false
```

## Notes

- Matches on the exact declared `name`, not on the tables it connects — two different
  tables pairs could share a lookup pattern, but each relationship still needs its own
  unique name (enforced by [`add_relationship()`](add_relationship.md)).
- Never throws, regardless of the name given.

## See Also

- [`Nexus::relationship`](relationship.md): fetch the relationship itself once you know it
  exists.
- [`Nexus::add_relationship`](add_relationship.md): declare a relationship, including the
  uniqueness rule this method lets you check ahead of time.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="has_store.html">
    <span class="label">Next</span>
    <span class="title">has_store</span>
  </a>
</div>
