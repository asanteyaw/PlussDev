<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 62bf86d0fb06 -->

# Archive::nrecords

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
std::int64_t Archive::nrecords() const
```

## What It Does

Returns the number of records (rows) per catalog — **only valid when every catalog in the
archive has the same record count**. If catalogs disagree, it throws rather than picking
one; use [`minrecords()`](minrecords.md)/[`maxrecords()`](maxrecords.md) for a ragged
archive.

```{important} Throws on a ragged archive — a distinct check from nkeys()
`nrecords()` requires every catalog to have the *same number* of records; a mismatch throws
`"Archive::nrecords: catalogs have different record counts"`. This is a separate check from
[`nkeys()`](nkeys.md) — catalogs can agree on record count while disagreeing on key count,
or vice versa.
```

## Parameters

`None`

## Returns

`std::int64_t`
    The (uniform) number of records per catalog.

## Examples

Example 1. Every catalog has the same record count.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({4.0, 5.0, 6.0})),
});
auto archive = pluss::kindle::Archive({c0, c1});

std::cout << archive.nrecords();
```

### Output

```text
3
```

Example 2. Catalogs with different record counts throw instead of silently picking one.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({4.0, 5.0})),
});
auto ragged = pluss::kindle::Archive({c0, c1});

try {
  ragged.nrecords();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Archive::nrecords: catalogs have different record counts
```

## Notes

- Requires uniform record count across every catalog — see the important callout above.
- Requires a defined archive; throws `"Archive: undefined handle"` on a default-constructed
  `Archive()`.

## See Also

- [`Archive::minrecords`](minrecords.md) / [`Archive::maxrecords`](maxrecords.md): tolerate
  a ragged archive by reporting the range instead of throwing.
- [`Archive::nkeys`](nkeys.md): the equivalent uniform-only check for key count.
- [`Archive::shape`](shape.md): includes this method's value as its second element.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="rename.html">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>
