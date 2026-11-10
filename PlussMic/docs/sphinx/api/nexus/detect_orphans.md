<!-- pluss-hand-authored -->

# Nexus::detect_orphans

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::detect_orphans(std::string_view relationship_name, RelationshipSide side = RelationshipSide::Left) const -> DataFrame
```

## What It Does

Returns the rows on one side of a declared relationship that have no match on the other
side — the actual rows behind [`validate_relationship()`](validate_relationship.md)'s
coverage check. `side` picks which table's unmatched rows you get back:

### `RelationshipSide` values

| Value | Numeric value | Description |
|---|---|---|
| `Left` | `0` | — |
| `Right` | `1` | — |

`RelationshipSide::Left` (the default) returns rows from `Relationship::left_table` that
have no matching row in `right_table`; `Right` returns the mirror image — rows from
`right_table` with no match in `left_table`.

## Parameters

**relationship_name** (`std::string_view`)
    The relationship to check, matching [`Relationship::name`](relationships.md). Must
    already be declared — an unknown name throws
    `"Nexus::detect_orphans: unknown relationship '<name>'"`.

**side** (`RelationshipSide`, default `Left`)
    Which side's unmatched rows to return.

## Returns

`DataFrame`
    The unmatched rows from the requested side, with the same features as that side's own
    table. Empty (but defined) if every row on that side has a match.

## Examples

Example 1. An order referencing a customer that doesn't exist.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben"}))},
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

auto linked = biz.add_relationship(rel);

auto left_orphans = linked.detect_orphans("orders_to_customers", RelationshipSide::Left);
std::cout << left_orphans.ncases() << "\n";
std::cout << left_orphans.features("order_id");
```

### Output

```text
1
0  103
Name: order_id, ptype: Series
```

Order `103` (the third row, at position `0` in the returned frame) references
`customer_id = 4.0`, which has no matching row in `customers`.

Example 2. The `Right` side, using `RelationshipSide::Left`'s default omitted — every
customer with no orders at all would show up here; in this data every customer has at
least one order, so it's empty.

```cpp
auto right_orphans = linked.detect_orphans("orders_to_customers", RelationshipSide::Right);
std::cout << right_orphans.ncases();
```

### Output

```text
0
```

## Notes

- `side` defaults to `RelationshipSide::Left`.
- Returned rows use the source table's own features and original row content — no key
  renaming or suffixing.
- An unknown `relationship_name` throws; there's no `std::optional`-returning form.

## See Also

- [`Nexus::orphans`](orphans.md): an exact alias.
- [`Nexus::validate_relationship`](validate_relationship.md): the coverage check these
  rows feed into, collapsed to a `bool`.
- [`Nexus::relationship_audit`](relationship_audit.md): orphan counts for both sides at
  once, alongside every other relationship-health metric.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="detect_duplicates.html">
    <span class="label">Previous</span>
    <span class="title">detect_duplicates</span>
  </a>
  <a class="next-link" href="dloc.html">
    <span class="label">Next</span>
    <span class="title">dloc</span>
  </a>
</div>
