<!-- pluss-hand-authored -->

# Locus::fingerprint

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
std::string Locus::fingerprint() const
```

## What It Does

Calls `Nexus::fingerprint` on this `Locus`'s aligned [`nexus()`](nexus.md)
view: a deterministic hash summarizing the whole workspace's content --
every catalog's name, shape, feature names, and dtypes; every declared
[`TableMeta`](meta.md); every declared [`Relationship`](link.md); and every
[saved plan](plan.md) -- as a short hex string. Two workspaces with the
same data, metadata, links, and plans produce the identical fingerprint,
regardless of which separately-constructed `Locus` produced them; changing
any of that content changes it.

## Parameters

None.

## Returns

A hex-string hash.

## Examples

Example 1. The fingerprint is stable across repeated calls, identical for
two independently-built but equal workspaces, and changes when the data
does.

```cpp
auto shop = make_linked_shop();
TableMeta meta;
meta.primary_key = {"id"};
shop.meta_("customers", meta);
const auto fp = shop.fingerprint();
std::cout << "fingerprint stable: " << (fp == shop.fingerprint()) << "\n";

auto same_shop = make_linked_shop();
same_shop.meta_("customers", meta);
std::cout << "equal workspaces share fingerprint: " << (fp == same_shop.fingerprint()) << "\n";

std::cout << "changed data changes fingerprint: "
          << (fp != shop.with("customers", shop.catalog("orders")).fingerprint()) << "\n";
```

### Output

```text
fingerprint stable: true
equal workspaces share fingerprint: true
changed data changes fingerprint: true
```

## See Also

- [`alignment_cache_stats`](alignment_cache_stats.md): a different kind of
  fingerprint (per-catalog, for cache keys) used internally for alignment
  reuse.
