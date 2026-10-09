<!-- pluss-hand-authored -->

# Nexus::links

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
Nexus::links() const -> const std::vector<Relationship>&

Nexus::links(const Relationship& rel) const -> Nexus
Nexus& Nexus::links_(const Relationship& rel)

Nexus::links(std::initializer_list<Relationship> rels) const -> Nexus
Nexus& Nexus::links_(std::initializer_list<Relationship> rels)
```

## What It Does

A family of exact aliases: the zero-argument `links()` calls
[`relationships()`](relationships.md); the single-`Relationship` and
`initializer_list<Relationship>` overloads of `links`/`links_` call
[`add_relationship`/`add_relationship_`/`add_relationships`/`add_relationships_`](add_relationship.md)
respectively. All behave identically to their canonical counterparts — same validation,
same throws, same non-mutating/mutating split. `links` exists purely as a shorter,
more conversational spelling for the same four calls.

## Parameters

See [`relationships()`](relationships.md) (zero-argument overload) and
[`add_relationship()`](add_relationship.md) (the four add overloads) for the full parameter
documentation — identical here.

## Returns

Identical to [`relationships()`](relationships.md) and
[`add_relationship()`](add_relationship.md).

## Examples

Example 1. `links()` and `relationships()` always agree; `links(rel)` behaves exactly like
`add_relationship(rel)`.

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

std::cout << (biz.links().size() == biz.relationships().size()) << "\n";

Relationship rel;
rel.name = "orders_to_customers";
rel.left_table = "orders";
rel.right_table = "customers";
rel.keys = {{"customer_id", "customer_id"}};

auto linked = biz.links(rel);
std::cout << linked.nrelationships();
```

### Output

```text
1
1
```

## Notes

- Purely a naming alternative — see [`relationships()`](relationships.md) and
  [`add_relationship()`](add_relationship.md) for the full documentation, including
  validation rules and throw conditions.

## See Also

- [`Nexus::relationships`](relationships.md): the canonical getter this aliases.
- [`Nexus::add_relationship`](add_relationship.md): the canonical setter family this
  aliases.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keys.html">
    <span class="label">Previous</span>
    <span class="title">keys</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
