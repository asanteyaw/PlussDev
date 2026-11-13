<!-- pluss-hand-authored -->

# Nexus::validate_relationship

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
bool Nexus::validate_relationship(std::string_view name) const
```

## What It Does

Checks whether a declared [`Relationship`](relationships.md) actually holds against the
current data, collapsed to a single `bool`. This is the relationship-level counterpart to
[`validate_keys()`](validate_keys.md); [`relationship_audit()`](relationship_audit.md) is
the version that returns the full breakdown instead of just the final verdict.

A relationship is considered valid when **all** of the following hold:

- If `cardinality` requires the left side to be unique (`OneToOne` or `OneToMany`), the
  left table's declared key must have no duplicates among rows that actually match the
  relationship.
- The same uniqueness requirement, mirrored for the right side under `OneToOne` or
  `ManyToOne`.
- Unless `right_optional` is `true`, every left-side row must have a matching right-side
  row (no missing keys, no orphans).
- Unless `left_optional` is `true`, every right-side row must have a matching left-side row.

## Parameters

**name** (`std::string_view`)
    The relationship to validate, matching [`Relationship::name`](relationships.md). Must
    already be declared — an unknown name throws
    `"Nexus::validate_relationship: unknown relationship '<name>'"`.

## Returns

`bool`
    `true` if the relationship holds against the current data under all of the rules
    above, `false` otherwise.

## Examples

Example 1. A relationship where one side has an unmatched row (an orphan).

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy", "Cy2"}))},
});
auto orders = DataFrame({
    {"order_id",    Slate(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 4.0}))},
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

std::cout << std::boolalpha << linked.validate_relationship("orders_to_customers");
```

### Output

```text
false
```

Order `103` references `customer_id = 4.0`, which doesn't exist in `customers` — an
orphan on the left side, and `right_optional` defaults to `false`, so the relationship
doesn't validate.

Example 2. An unknown relationship name throws.

```cpp
try {
  auto bad = linked.validate_relationship("nope");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::validate_relationship: unknown relationship 'nope'
```

## Notes

- Combines uniqueness (driven by `cardinality`) and coverage (driven by
  `left_optional`/`right_optional`) into a single verdict — see
  [`relationship_audit()`](relationship_audit.md) for the itemized breakdown.
- [`check()`](check.md) is an exact alias for this method.
- Results are cached internally per relationship name.

## See Also

- [`Nexus::relationship_audit`](relationship_audit.md): the same check, returning every
  intermediate count instead of just the final `bool`.
- [`Nexus::detect_orphans`](detect_orphans.md): the actual unmatched rows on either side.
- [`Nexus::check`](check.md): an exact alias.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="validate_keys.html">
    <span class="label">Previous</span>
    <span class="title">validate_keys</span>
  </a>
</div>
