<!-- pluss-hand-authored -->

# Locus::graph

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::graph() const -> pluss::table::DataFrame
```

## What It Does

Calls `Nexus::graph` on this `Locus`'s aligned [`nexus()`](nexus.md) view:
renders every declared [`Relationship`](link.md), one row per relationship
-- the table-free counterpart to [`schema`](schema.md). This is the format
[`save()`](persistence.md) writes to disk and [`read()`](persistence.md)
reads back to reconstruct a workspace's declared relationships.

## Parameters

None.

## Returns

A `DataFrame` with one row per declared relationship.

## Examples

Example 1. Two declared relationships, two rows.

```cpp
const auto linked = make_linked_shop();
std::cout << "graph ncases=" << linked.graph().ncases() << "\n";
```

### Output

```text
graph ncases=2
```

## See Also

- [`schema`](schema.md): the table-level counterpart.
- [`link`](link.md): declaring the relationships this report renders.
