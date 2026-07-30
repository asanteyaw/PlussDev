<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a7c3b4f69ca0 -->

# Archive::maxkeys

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::maxkeys() const -> std::int64_t
```

## What It Does

Returns the largest key count among the archive's catalogs — the mirror of
[`minkeys()`](minkeys.md), and the ragged-tolerant counterpart to [`nkeys()`](nkeys.md).
Never throws for a key-count mismatch; on a uniform archive, `maxkeys() == minkeys() ==
nkeys()`.

## Parameters

`None`

## Returns

`std::int64_t`
    The largest key count found across every catalog in the archive.

## Examples

Example 1. A ragged archive: one catalog has 1 key, the other has 2.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Ho", "Koforidua"})),
});
auto ragged = pluss::kindle::Archive({c0, c1});

std::cout << ragged.maxkeys();
```

### Output

```text
2
```

## Notes

- Never throws for a key-count mismatch, unlike [`nkeys()`](nkeys.md).
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::minkeys`](minkeys.md): the mirror — the smallest key count instead of the
  largest.
- [`Archive::nkeys`](nkeys.md): the uniform-only equivalent, which throws instead of
  tolerating a ragged archive.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map_catalogs.html">
    <span class="label">Previous</span>
    <span class="title">map_catalogs</span>
  </a>
  <a class="next-link" href="maxrecords.html">
    <span class="label">Next</span>
    <span class="title">maxrecords</span>
  </a>
</div>
