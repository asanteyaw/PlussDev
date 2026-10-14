<!-- pluss-hand-authored -->

# Locus::alignment_cache_stats

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::alignment_cache_stats() const -> std::pair<std::int64_t, std::int64_t>
```

## What It Does

Reports `(hits, misses)` of the *per-catalog* alignment cache behind
[`nexus()`](nexus.md). A catalog is tabelized once per `TabelizePolicy` and
the aligned result is cached, keyed by the catalog's own data fingerprint
plus the policy -- not by which `Locus` asked for it, and not by the
catalog's position or name. That cache is shared across every `Locus`
derived from a common ancestor (`with_`, `link_`, and the rest), so a
catalog aligned once by one `Locus` is reused by another that happens to
hold the identical data under the same policy, even if that catalog arrived
through a different-looking derivation.

This counts per-catalog alignments, **not** `nexus()` calls: a second
`nexus()` call on the *same* `Locus` with the *same* policy is answered
directly from that `Locus`'s own cached `Nexus` (see [`nexus`](nexus.md))
and never reaches the per-catalog cache at all, so it changes neither `hits`
nor `misses`. A hit only appears when a *different*, derived `Locus` calls
`nexus()` for the first time and finds that some of its catalogs were
already aligned by an ancestor it shares data with.

## Parameters

None.

## Returns

`{hits, misses}`, both `std::int64_t`, as of the moment of the call.

## Examples

Example 1. A fresh `Locus`'s first `nexus()` call is all misses -- one per
catalog. A second call on the *same* `Locus` changes nothing, since it's
answered from that `Locus`'s own cached `Nexus` before the per-catalog cache
is ever consulted.

```cpp
auto base = make_linked_shop();
const auto before = base.alignment_cache_stats();
std::cout << "before any nexus(): hits=" << before.first << " misses=" << before.second << "\n";
(void)base.nexus();
const auto after_first = base.alignment_cache_stats();
std::cout << "after 1st nexus(): hits=" << after_first.first << " misses=" << after_first.second << "\n";
(void)base.nexus();
const auto after_second = base.alignment_cache_stats();
std::cout << "after 2nd nexus() on the SAME Locus: hits=" << after_second.first
          << " misses=" << after_second.second << "\n";
```

### Output

```text
before any nexus(): hits=0 misses=0
after 1st nexus(): hits=0 misses=3
after 2nd nexus() on the SAME Locus: hits=0 misses=3
```

Three catalogs (`customers`, `orders`, `items`), three misses on the first
alignment. The second call doesn't add three more misses -- it adds
nothing, because it never reaches the per-catalog cache.

Example 2. A *derived* `Locus` that shares data with `base` gets hits on
its own first `nexus()` call -- even for a catalog `with_` just "replaced"
with the identical data it already held.

```cpp
auto derived = base;
derived.with_("items", base.catalog("items"));
(void)derived.nexus();
const auto after_derived = derived.alignment_cache_stats();
std::cout << "after a derived Locus's nexus(): hits=" << after_derived.first
          << " misses=" << after_derived.second << "\n";
```

### Output

```text
after a derived Locus's nexus(): hits=3 misses=3
```

All three of `derived`'s catalogs hit the cache -- not just the two
(`customers`, `orders`) that were never touched, but also `items`, because
`with_("items", base.catalog("items"))` replaced it with data that
fingerprints identically to what was already cached. The cache keys on data,
not on which `Locus` or which call produced it.

## See Also

- [`nexus`](nexus.md): the whole-`Nexus`-level cache this sits underneath,
  and the one a repeated call on the same `Locus` actually hits.
- [`policy`](policy.md): part of the cache key alongside each catalog's own
  data.
