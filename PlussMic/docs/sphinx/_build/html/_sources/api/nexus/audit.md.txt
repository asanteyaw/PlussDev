<!-- pluss-hand-authored -->

# Nexus::audit

[Back to Nexus API Reference](../../nexus_api_reference)

## Signature

```cpp
Nexus::audit() const -> DataFrame
Nexus::audit(std::string_view name) const -> DataFrame
```

## What It Does

`audit()` now has two roles:

- **`audit()` with no arguments** returns the workspace-wide integrity table — the same
  combined table/relationship diagnostics returned by [`scan()`](scan.md)
- **`audit(name)`** remains the shorter alias for
  [`relationship_audit(name)`](relationship_audit.md)

That split makes `audit()` the natural trust entry point for `Nexus`: one call to inspect
the whole workspace, or one named call to zoom into a single declared relationship.

## Parameters

For `audit()`: none.

For `audit(name)`: see [`relationship_audit()`](relationship_audit.md) — identical here.

## Returns

For `audit()`: the same combined diagnostics frame as [`scan()`](scan.md).

For `audit(name)`: identical to [`relationship_audit()`](relationship_audit.md).

## Examples

Example 1. `audit()` with no arguments returns the workspace command-center audit.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

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

std::cout << linked.audit().features("kind");
```

### Output

```text
0  table
1  table
2  relationship
Name: kind, ptype: Note
```

Example 2. `audit(name)` still aliases the one-relationship summary.

```cpp
std::cout << linked.audit("orders_to_customers").features("valid");
```

### Output

```text
0  true
Name: valid, ptype: Series
```

## Notes

- `audit()` is now the short workspace-integrity entry point; it is not relationship-only
  anymore.
- `audit(name)` is still purely a naming alternative for
  [`relationship_audit(name)`](relationship_audit.md).

## See Also

- [`Nexus::scan`](scan.md): the canonical workspace-level audit table returned by `audit()`.
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
