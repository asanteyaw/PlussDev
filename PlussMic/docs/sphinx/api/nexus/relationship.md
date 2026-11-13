<!-- pluss-hand-authored -->

# Nexus::relationship

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
std::optional<Relationship> Nexus::relationship(std::string_view name) const
```

## What It Does

Looks up one declared [`Relationship`](relationships.md) by name. Unlike most single-item
lookups elsewhere on this class ([`frame(key)`](frame.md), which throws on an unknown key),
`relationship()` follows [`table_meta()`](table_meta.md)'s pattern: an unknown name returns
`std::nullopt` rather than throwing.

## Parameters

**name** (`std::string_view`)
    The relationship name to look up, matching [`Relationship::name`](relationships.md).

## Returns

`std::optional<Relationship>`
    The full declaration if `name` is known, `std::nullopt` otherwise.

## Examples

Example 1. A known relationship returns its full declaration.

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
auto found = linked.relationship("orders_to_customers");

std::cout << std::boolalpha << found.has_value() << " " << found->left_table << " -> " << found->right_table;
```

### Output

```text
true orders -> customers
```

Example 2. An unknown name returns `std::nullopt`, not a throw.

```cpp
auto missing = linked.relationship("nope");
std::cout << std::boolalpha << missing.has_value();
```

### Output

```text
false
```

## Notes

- Returns `std::nullopt` for an unknown name; never throws.
- Returns a copy of the `Relationship`, not a reference — cheap enough given
  `Relationship`'s small size, and lets the result outlive the workspace it came from
  without dangling.

## See Also

- [`Nexus::has_relationship`](has_relationship.md): check existence without unwrapping the
  `std::optional`.
- [`Nexus::relationships`](relationships.md): every declared relationship at once.
- [`Nexus::add_relationship`](add_relationship.md): declare a new relationship.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pull.html">
    <span class="label">Previous</span>
    <span class="title">pull</span>
  </a>
  <a class="next-link" href="relationship_audit.html">
    <span class="label">Next</span>
    <span class="title">relationship_audit</span>
  </a>
</div>
