<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8c476aea47bd -->

# Archive::maxrecords

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::maxrecords() const -> std::int64_t
```

## What It Does

Returns the largest record count among the archive's catalogs — the mirror of
[`minrecords()`](minrecords.md), and the ragged-tolerant counterpart to
[`nrecords()`](nrecords.md). Never throws for a record-count mismatch; on a uniform
archive, `maxrecords() == minrecords() == nrecords()`.

## Parameters

`None`

## Returns

`std::int64_t`
    The largest record count found across every catalog in the archive.

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

std::cout << ragged.maxrecords();
```

### Output

```text
3
```

## Notes

- Never throws for a record-count mismatch, unlike [`nrecords()`](nrecords.md).
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::minrecords`](minrecords.md): the mirror — the smallest record count instead of
  the largest.
- [`Archive::nrecords`](nrecords.md): the uniform-only equivalent, which throws instead of
  tolerating a ragged archive.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="maxkeys.html">
    <span class="label">Previous</span>
    <span class="title">maxkeys</span>
  </a>
  <a class="next-link" href="minkeys.html">
    <span class="label">Next</span>
    <span class="title">minkeys</span>
  </a>
</div>
