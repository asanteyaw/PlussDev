<!-- pluss-hand-authored -->

# Nexus::add_relationship

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
Nexus::add_relationship(const Relationship& rel) const -> Nexus
Nexus& Nexus::add_relationship_(const Relationship& rel)

Nexus::add_relationships(std::initializer_list<Relationship> rels) const -> Nexus
Nexus& Nexus::add_relationships_(std::initializer_list<Relationship> rels)
```

## What It Does

Declares one or more relationships between tables already in the workspace. Follows this
class's usual naming and mutability conventions: singular `add_relationship` for one
relationship at a time, plural `add_relationships` for a batch in one call; the plain form
returns a new `Nexus` and the `_`-suffixed form mutates the current handle in place.

Declaring a relationship never touches the underlying tables or their data — it only
records an edge in the workspace's relationship graph, which downstream methods
([`path()`](path.md), [`plan()`](plan.md), [`neighbors()`](neighbors.md),
[`relationship_audit()`](relationship_audit.md), [`pull()`](pull.md),
[`NexusTraverse::join`](../../nexus_traverse_api_reference)) then use to find and validate join
paths without you having to restate the join keys every time.

## Parameters

**rel** (`const Relationship&`, singular overload)
    The relationship to declare. See [`relationships()`](relationships.md) for the full
    field-by-field breakdown of `Relationship`. Validated before being added:

    - `name` must be non-empty and not already used by another declared relationship in
      this workspace.
    - `left_table` and `right_table` must both be non-empty and must both already exist in
      the workspace.
    - `keys` must contain at least one `(left_feature, right_feature)` pair, and no pair
      may have an empty feature name on either side.
    - `confidence` must be in `[0, 1]`.

    Note that the named features in `keys` are **not** checked against the tables'
    actual columns at declaration time — only when something actually joins on them (see
    [`join()`](join.md)/[`materialize()`](materialize.md)).

**rels** (`std::initializer_list<Relationship>`, plural overload)
    A batch of relationships, each validated the same way as the singular overload and
    added in the order given. If any entry fails validation, none of the batch's earlier
    entries are un-added on the mutating `_` overload — validation happens as each entry is
    processed, so a failure partway through the list leaves the workspace with however many
    relationships were successfully added before the failure. Prefer the non-mutating form
    if you need all-or-nothing behavior; check each `Relationship` yourself first, or wrap
    the call and discard the result on failure.

## Returns

For the plain overloads: `Nexus`
    A new workspace with the relationship(s) added; the source `Nexus` is unchanged.

For the `_`-suffixed overloads: `Nexus&`
    A reference to the same, now-mutated workspace.

## Examples

Example 1. Declare a single relationship, non-mutating form.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name",        Thread(pluss::make_note({"Ada", "Ben", "Cy"}))},
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
rel.cardinality = RelationshipCardinality::ManyToOne;

auto linked = biz.add_relationship(rel);

std::cout << biz.nrelationships() << " " << linked.nrelationships();
```

### Output

```text
0 1
```

`biz` is left with no relationships — `add_relationship()` is non-mutating.

Example 2. `add_relationship_` mutates the current handle in place.

```cpp
auto mutable_biz = biz;
mutable_biz.add_relationship_(rel);

std::cout << biz.nrelationships() << " " << mutable_biz.nrelationships();
```

### Output

```text
0 1
```

Example 3. Declaring a relationship with a name that's already in use throws.

```cpp
try {
  auto dup = linked.add_relationship(rel);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Nexus::add_relationship: duplicate relationship 'orders_to_customers'
```

Example 4. Validation failures: empty name, unknown table, empty key list.

```cpp
try {
  Relationship bad;
  bad.name = "";
  bad.left_table = "orders";
  bad.right_table = "customers";
  bad.keys = {{"customer_id", "customer_id"}};
  auto x = biz.add_relationship(bad);
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}

try {
  Relationship bad;
  bad.name = "r2";
  bad.left_table = "orders";
  bad.right_table = "missing_table";
  bad.keys = {{"customer_id", "customer_id"}};
  auto x = biz.add_relationship(bad);
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}

try {
  Relationship bad;
  bad.name = "r3";
  bad.left_table = "orders";
  bad.right_table = "customers";
  auto x = biz.add_relationship(bad);
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
Nexus::add_relationship: relationship name cannot be empty
Nexus::add_relationship: unknown table 'missing_table'
Nexus::add_relationship: relationship keys cannot be empty
```

Example 5. Declare several relationships in one batch call.

```cpp
Relationship second;
second.name = "orders_to_customers_2";
second.left_table = "orders";
second.right_table = "customers";
second.keys = {{"customer_id", "customer_id"}};

auto batch = biz.add_relationships({rel, second});
std::cout << batch.nrelationships();
```

### Output

```text
2
```

## Notes

- `name` must be unique across the whole workspace; both tables named must already exist.
- `keys` feature names are validated for non-emptiness only at declaration time — not
  checked against the tables' actual columns until something joins on them.
- The batch (`add_relationships`) overload validates and inserts one entry at a time; a
  failure partway through leaves the mutating handle with the entries added before the
  failure — see the Parameters section above.
- [`links()`](links.md)/[`links_()`](links.md) are exact aliases for these four overloads.

## See Also

- [`Nexus::relationships`](relationships.md): read back every declared relationship,
  including the full `Relationship` field breakdown.
- [`Nexus::relationship`](relationship.md) / [`has_relationship`](has_relationship.md):
  look up one relationship by name.
- [`Nexus::deduce`](deduce.md): suggest relationships automatically instead of declaring
  them by hand.
- [`Nexus::relationship_audit`](relationship_audit.md): check whether a declared
  relationship actually holds against the current data.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="apply.html">
    <span class="label">Next</span>
    <span class="title">apply</span>
  </a>
</div>
