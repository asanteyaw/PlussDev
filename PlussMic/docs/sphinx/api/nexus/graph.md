<!-- pluss-hand-authored -->

# Nexus::graph

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::graph() const -> DataFrame
```

## What It Does

Renders every declared [`Relationship`](relationships.md) as a `DataFrame`, one row per
relationship — the table-free counterpart to [`schema()`](schema.md). This is the format
[`save()`](save.md) writes to disk (as `graph.csv` by default) and [`load()`](load.md)
reads back to reconstruct declared relationships.

## Parameters

`None`

## Returns

`DataFrame`
    One row per declared relationship, with columns `name`, `left`, `right`, `keys`
    (encoded key pairs), `cardinality`, `left_optional`, `right_optional`, `confidence`,
    `status`, and `note` — directly mirroring [`Relationship`](relationships.md)'s own
    fields.

## Examples

Example 1. One declared relationship.

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
});
auto orders = DataFrame({
    {"order_id",    Thread(pluss::make_series({101.0, 102.0, 103.0}))},
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 2.0}))},
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

auto g = linked.graph();
std::cout << g.features({"name", "left", "right", "cardinality"});
```

### Output

```text
==========================================================
     | name                 left    right      cardinality
----------------------------------------------------------
0    | orders_to_customers  orders  customers  many_to_one
----------------------------------------------------------
[1 cs x 4 feat]
```

## Notes

- This is exactly the format [`save()`](save.md)/[`load()`](load.md) round-trip through
  disk for relationships.
- Empty (no declared relationships) returns a defined, zero-row frame rather than throwing.

## See Also

- [`Nexus::schema`](schema.md): the table-level counterpart to this relationship-level
  view.
- [`Nexus::relationships`](relationships.md): the underlying `Relationship` values, as a
  `std::vector` rather than a `DataFrame`.
- [`Nexus::neighbors`](neighbors.md): relationships reoriented from one table's own
  perspective.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="get.html">
    <span class="label">Previous</span>
    <span class="title">get</span>
  </a>
  <a class="next-link" href="grain_audit.html">
    <span class="label">Next</span>
    <span class="title">grain_audit</span>
  </a>
</div>
