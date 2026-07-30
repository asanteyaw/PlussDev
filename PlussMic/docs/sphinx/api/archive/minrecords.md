<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 07846c0d126f -->

# Archive::minrecords

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::minrecords() const -> std::int64_t
```

## What It Does

Returns the smallest record count among the archive's catalogs — the ragged-tolerant
counterpart to [`nrecords()`](nrecords.md), which throws if catalogs disagree. Never throws
for a record-count mismatch; on a uniform archive, `minrecords() == maxrecords() ==
nrecords()`.

## Parameters

`None`

## Returns

`std::int64_t`
    The smallest record count found across every catalog in the archive.

## Examples

Example 1. A ragged archive: one catalog has 3 records, the other has 2.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({4.0, 5.0})),
});
auto ragged = pluss::kindle::Archive({c0, c1});

std::cout << ragged.minrecords();
```

### Output

```text
2
```

## Notes

- Never throws for a record-count mismatch, unlike [`nrecords()`](nrecords.md).
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::maxrecords`](maxrecords.md): the mirror — the largest record count instead of
  the smallest.
- [`Archive::nrecords`](nrecords.md): the uniform-only equivalent, which throws instead of
  tolerating a ragged archive.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minkeys.html">
    <span class="label">Previous</span>
    <span class="title">minkeys</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
