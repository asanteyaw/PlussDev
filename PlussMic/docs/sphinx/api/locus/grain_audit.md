<!-- pluss-hand-authored -->

# Locus::grain_audit

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
pluss::table::DataFrame Locus::grain_audit(std::string_view table) const
```

## What It Does

Calls `Nexus::grain_audit` on this `Locus`'s aligned [`nexus()`](nexus.md)
view: runs the same completeness-and-uniqueness check as
[`validate_keys`](audit.md) against *every* declared key set on `table` --
the primary key plus every candidate key -- and returns the results as one
row per key set, so several key sets can be compared side by side.

## Parameters

| Parameter | Meaning |
|---|---|
| `table` | The catalog whose declared key sets to check. |

## Returns

A `DataFrame` with one row per declared key set on `table`.

## Examples

Example 1. One row for `customers`' declared primary key.

```cpp
auto shop = make_linked_shop();
TableMeta meta;
meta.primary_key = {"id"};
shop.meta_("customers", meta);
std::cout << "grain_audit ncases=" << shop.grain_audit("customers").ncases() << "\n";
```

### Output

```text
grain_audit ncases=1
```

## See Also

- [`relationship_audit`](relationship_audit.md): the matching one-table-row
  report at the relationship level.
- [`audit`](audit.md): `validate_keys`, the single-key-set check this report
  runs over every declared key set at once.
