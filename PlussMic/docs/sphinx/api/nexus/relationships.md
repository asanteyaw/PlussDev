<!-- pluss-hand-authored -->

# Nexus::relationships

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
const std::vector<Relationship>& Nexus::relationships() const
```

## What It Does

Returns every declared `Relationship` in the workspace, in the order they were added. A
`Relationship` is a declared edge between two tables:

### `Relationship` fields

| Field | Type | Default |
|---|---|---|
| `name` | `std::string` | empty |
| `left_table` | `std::string` | empty |
| `right_table` | `std::string` | empty |
| `keys` | `std::vector<std::pair<std::string, std::string>>` | empty |
| `cardinality` | `RelationshipCardinality` | `RelationshipCardinality::ManyToOne` |
| `left_optional` | `bool` | `false` |
| `right_optional` | `bool` | `false` |
| `confidence` | `double` | `1.0` |
| `status` | `RelationshipValidationStatus` | `RelationshipValidationStatus::Declared` |
| `note` | `std::optional<std::string>` | empty |

`name` uniquely identifies the relationship (see [`add_relationship()`](add_relationship.md)
for the uniqueness rule). `left_table`/`right_table` name the two tables it connects.
`keys` is a list of `(left_feature, right_feature)` pairs — most relationships have exactly
one pair, but composite keys are supported. `cardinality` declares the expected shape
(`OneToOne`, `OneToMany`, `ManyToOne`, `ManyToMany`) — consumed by
[`relationship_audit()`](relationship_audit.md) to decide whether uniqueness is required on
each side. `left_optional`/`right_optional` mark whether every row on that side is expected
to have a match on the other (an unmatched row is only counted as a problem by
`relationship_audit()` when the corresponding side is *not* optional). `confidence` and
`status` are descriptive fields — set directly here, or computed for you by
[`deduce()`](deduce.md) and [`scan()`](scan.md) respectively; `Nexus` itself never adjusts
a *declared* relationship's `status` in place. `note` is free text.

This is the low-level read; [`neighbors()`](neighbors.md) and [`graph()`](graph.md) present
the same declarations reshaped as `DataFrame`s for easier inspection.

## Parameters

`None`

## Returns

`const std::vector<Relationship>&`
    Every declared relationship, in declaration order — empty if none have been added.

## Examples

Example 1. An empty workspace declares no relationships.

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

std::cout << biz.relationships().size();
```

### Output

```text
0
```

Example 2. After declaring one relationship.

```cpp
Relationship rel;
rel.name = "orders_to_customers";
rel.left_table = "orders";
rel.right_table = "customers";
rel.keys = {{"customer_id", "customer_id"}};
rel.cardinality = RelationshipCardinality::ManyToOne;

auto linked = biz.add_relationship(rel);

std::cout << linked.relationships().size() << "\n";
std::cout << linked.relationships().front().name << " "
          << linked.relationships().front().left_table << " -> "
          << linked.relationships().front().right_table;
```

### Output

```text
1
orders_to_customers orders -> customers
```

## Notes

- Returned by reference — no copy is made just to read the declarations.
- Declaration order, not any topological or alphabetical order.
- [`links()`](links.md) is an exact alias for this same method.
- `Nexus` never rewrites a declared relationship's `status` field for you — that field
  reflects what you (or [`scan()`](scan.md), when you read its output back) declared, not
  a live-computed validity check. For the live check, see
  [`relationship_audit()`](relationship_audit.md)'s `computed_status` column.

## See Also

- [`Nexus::links`](links.md): an exact alias.
- [`Nexus::nrelationships`](nrelationships.md): the count, without materializing this
  vector's contents.
- [`Nexus::add_relationship`](add_relationship.md): declare a new relationship.
- [`Nexus::relationship`](relationship.md): look up one relationship by name.
- [`Nexus::graph`](graph.md): the same declarations as a `DataFrame`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="relationship_audit.html">
    <span class="label">Previous</span>
    <span class="title">relationship_audit</span>
  </a>
  <a class="next-link" href="rename.html">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>
