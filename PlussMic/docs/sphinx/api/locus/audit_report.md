<!-- pluss-hand-authored -->

# Locus::audit_report

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::audit_report() const -> pluss::table::DataFrame
```

## What It Does

The workspace-wide integrity table -- `nexus().audit()`'s combined
table-and-relationship diagnostics, the same frame
[`schema`](schema.md)/[`scan`](scan.md) build on. One row covers each
catalog's own key validity, and one row covers each declared relationship's
[`audit()`](audit.md) check -- not just the relationships alone.

## Parameters

None.

## Returns

A `DataFrame` with one row per catalog plus one row per declared
relationship.

## Examples

Example 1. Three catalogs and two declared relationships: five rows.

```cpp
auto shop = make_linked_shop();
TableMeta meta;
meta.primary_key = {"id"};
shop.meta_("customers", meta);
std::cout << "audit_report ncases=" << shop.audit_report().ncases() << "\n";
```

### Output

```text
audit_report ncases=5
```

## See Also

- [`audit`](audit.md): the per-relationship check this report includes, as
  `LinkAudit` values for programmatic inspection instead of a `DataFrame`.
- [`relationship_audit`](relationship_audit.md): the same per-relationship
  row, scoped to one named relationship.
- [`schema`](schema.md), [`scan`](scan.md): the other reports built on this
  same combined diagnostics frame.
