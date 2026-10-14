<!-- pluss-hand-authored -->

# Locus::scan

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::scan() const -> pluss::table::DataFrame
```

## What It Does

Calls `Nexus::scan` on this `Locus`'s aligned [`nexus()`](nexus.md) view:
a unified diagnostics table mixing table rows and relationship rows in one
place -- the closest thing to a command-center overview of the whole
workspace. This is the same frame [`audit_report`](audit_report.md) returns.

## Parameters

None.

## Returns

A `DataFrame` with one row per catalog plus one row per declared
relationship.

## Examples

Example 1. Three catalogs plus two relationships: five rows -- the same
count [`audit_report`](audit_report.md) reports, since they're the same
underlying frame.

```cpp
auto shop = make_linked_shop();
TableMeta meta;
meta.primary_key = {"id"};
shop.meta_("customers", meta);
std::cout << "scan ncases=" << shop.scan().ncases() << "\n";
```

### Output

```text
scan ncases=5
```

## See Also

- [`audit_report`](audit_report.md): the same combined frame, under its
  integrity-focused name.
- [`profile`](profile.md): the table-only version of this diagnostics view.
