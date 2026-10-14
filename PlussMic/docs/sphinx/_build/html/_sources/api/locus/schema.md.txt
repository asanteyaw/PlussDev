<!-- pluss-hand-authored -->

# Locus::schema

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::schema() const -> pluss::table::DataFrame
```

## What It Does

Calls `Nexus::schema` on this `Locus`'s aligned [`nexus()`](nexus.md) view:
renders the workspace's table-level metadata, one row per catalog --
structural facts (row/column counts, feature names, dtypes) combined with
whatever [`TableMeta`](meta.md) has been declared. This is the format
[`save()`](persistence.md) writes to disk and [`read()`](persistence.md)
reads back to reconstruct a workspace's table metadata.

## Parameters

None.

## Returns

A `DataFrame` with one row per catalog.

## Examples

Example 1. One row per catalog.

```cpp
const auto linked = make_linked_shop();
std::cout << "schema ncases=" << linked.schema().ncases() << "\n";
```

### Output

```text
schema ncases=3
```

## See Also

- [`graph`](graph.md): the table-free, relationship-only counterpart.
- [`persistence`](persistence.md): where `schema()`'s format is written to
  and read from disk.
