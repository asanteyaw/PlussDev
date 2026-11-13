<!-- pluss-hand-authored -->

# Locus::nexus

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Nexus Locus::nexus(const Policy& policy = std::nullopt) const
```

```cpp
using Policy = std::optional<TabelizePolicy>;
```

## What It Does

Returns the aligned `DataFrame` workspace reports, SQL
([`query()`](query.md)), and `nexus()`-based delegation use -- every catalog
tabelized into a rectangular table under the given (or this `Locus`'s own)
`TabelizePolicy`, every declared link, table meta entry, and saved plan
carried over unchanged. The result is cached: calling `nexus()` again on the
*same* `Locus` with the *same* policy returns the cached `Nexus` directly,
without re-aligning anything -- see
[`alignment_cache_stats`](alignment_cache_stats.md) for the lower-level,
per-catalog cache this sits on top of.

## Parameters

| Parameter | Meaning |
|---|---|
| `policy` | The `TabelizePolicy` to align under. Defaults to this `Locus`'s own [`policy()`](policy.md). |

## Returns

A `Nexus` with one aligned `DataFrame` per catalog, and the same links,
table metadata, and saved plans.

## Examples

Example 1. `nexus()` carries over the workspace's tables and links.

```cpp
const auto linked = make_linked_shop();
const auto nx = linked.nexus();
std::cout << "nexus ntables=" << nx.ntables() << " nexus relationships=" << nx.relationships().size() << "\n";
```

### Output

```text
nexus ntables=3 nexus relationships=2
```

## See Also

- [`policy`](policy.md): the alignment `nexus()` uses by default.
- [`alignment_cache_stats`](alignment_cache_stats.md): what `nexus()`
  reuses, and when.
- [`query`](query.md): SQL over the same aligned view.
