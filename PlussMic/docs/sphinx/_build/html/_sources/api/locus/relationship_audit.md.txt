<!-- pluss-hand-authored -->

# Locus::relationship_audit

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::relationship_audit(std::string_view name) const -> pluss::table::DataFrame
```

## What It Does

Calls `Nexus::relationship_audit` on this `Locus`'s aligned
[`nexus()`](nexus.md) view: runs the full
[`validate_relationship`](audit.md) check for `name` and returns every
intermediate count (row counts, uniqueness, orphan counts on each side) as
a one-row `DataFrame`, instead of just the final `bool`
`validate_relationship` returns.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | The declared relationship to check. |

## Returns

A one-row `DataFrame` with that relationship's full audit breakdown.

## Examples

Example 1. One row, for one named relationship.

```cpp
auto shop = make_linked_shop();
TableMeta meta;
meta.primary_key = {"id"};
shop.meta_("customers", meta);
std::cout << "relationship_audit ncases=" << shop.relationship_audit("orders->customers").ncases() << "\n";
```

### Output

```text
relationship_audit ncases=1
```

## See Also

- [`audit`](audit.md): the same check, as a `LinkAudit` value instead of a
  `DataFrame` row.
- [`grain_audit`](grain_audit.md): the matching table-level report.
- [`audit_report`](audit_report.md): every relationship's audit, combined
  with every table's, in one workspace-wide report.
