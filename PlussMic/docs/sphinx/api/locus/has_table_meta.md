<!-- pluss-hand-authored -->

# Locus::has_table_meta, Locus::table_meta

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::has_table_meta(std::string_view name) const -> bool
Locus::table_meta(std::string_view name) const -> std::optional<TableMeta>
```

## What It Does

`has_table_meta` reports whether a catalog has a declared `TableMeta` entry
(grain, primary key, time field, provenance -- the shared vocabulary
[`Nexus`](../../nexus_api_reference.md) uses too). `table_meta` reads it
back, as `std::nullopt` when none is declared -- both are read-only probes;
see [`meta`](meta.md)/[`set_table_meta`](set_table_meta.md) for declaring
it.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | A catalog name. An unknown name is not an error -- it simply has no metadata. |

## Returns

`has_table_meta` returns `bool`. `table_meta` returns
`std::optional<TableMeta>`.

## Examples

Example 1. A catalog with declared metadata, and one without.

```cpp
TableMeta meta;
meta.grain = "one row per customer";
meta.primary_key = {"id"};
const auto linked = make_linked_shop();
const auto described = linked.meta("customers", meta);
std::cout << "described has_table_meta=" << described.has_table_meta("customers")
          << " original has_table_meta=" << linked.has_table_meta("customers") << "\n";
std::cout << "table_meta primary_key[0]=" << described.table_meta("customers")->primary_key[0] << "\n";
```

### Output

```text
described has_table_meta=true original has_table_meta=false
table_meta primary_key[0]=id
```

`meta(...)` (the setter) returns a new `Locus`, so `linked` itself never
gains the metadata -- only `described` does.

## See Also

- [`meta`](meta.md): the getter/setter pair this page's probes complement.
- [`set_table_meta`](set_table_meta.md): the explicitly-named setter.
