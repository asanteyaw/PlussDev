<!-- pluss-hand-authored -->

# Locus::graph

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
pluss::table::DataFrame Locus::graph() const
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

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="grain_audit.html">
    <span class="label">Previous</span>
    <span class="title">grain_audit</span>
  </a>
  <a class="next-link" href="has_store.html">
    <span class="label">Next</span>
    <span class="title">has_store</span>
  </a>
</div>
