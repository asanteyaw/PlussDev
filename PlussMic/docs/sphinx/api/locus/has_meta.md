<!-- pluss-hand-authored -->

# Locus::has_meta, Locus::table_meta

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
bool Locus::has_meta(std::string_view name) const
std::optional<TableMeta> Locus::table_meta(std::string_view name) const
```

## What It Does

`has_meta` reports whether a catalog has a declared `TableMeta` entry
(grain, primary key, time field, provenance -- the shared vocabulary
[`Nexus`](../../nexus_api_reference.md) uses too). `table_meta` reads it
back, as `std::nullopt` when none is declared -- both are read-only probes;
see [`meta`](meta.md) for declaring
it.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | A catalog name. An unknown name is not an error -- it simply has no metadata. |

## Returns

`has_meta` returns `bool`. `table_meta` returns
`std::optional<TableMeta>`.

## Examples

Example 1. A catalog with declared metadata, and one without.

```cpp
TableMeta meta;
meta.grain = "one row per customer";
meta.primary_key = {"id"};
const auto linked = make_linked_shop();
const auto described = linked.meta("customers", meta);
std::cout << "described has_meta=" << described.has_meta("customers")
          << " original has_meta=" << linked.has_meta("customers") << "\n";
std::cout << "table_meta primary_key[0]=" << described.table_meta("customers")->primary_key[0] << "\n";
```

### Output

```text
described has_meta=true original has_meta=false
table_meta primary_key[0]=id
```

`meta(...)` (the setter) returns a new `Locus`, so `linked` itself never
gains the metadata -- only `described` does.

## See Also

- [`meta`](meta.md): the getter/setter pair this page's probes complement.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="graph.html">
    <span class="label">Previous</span>
    <span class="title">graph</span>
  </a>
  <a class="next-link" href="has_store.html">
    <span class="label">Next</span>
    <span class="title">has_store</span>
  </a>
</div>
