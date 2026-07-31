<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 738f9c09c40f -->

# Archive::nkeys

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::nkeys() const -> std::int64_t
```

## What It Does

Returns the number of keys (streams) per catalog — **only valid when every catalog in the
archive has the same key count**. If catalogs disagree, it throws rather than picking one;
use [`minkeys()`](minkeys.md)/[`maxkeys()`](maxkeys.md) for a ragged archive.

```{important} Throws on a ragged archive — ragged in key count, not just record count
`nkeys()` requires every catalog to have the *same number* of keys; a mismatch throws
`"Archive::nkeys: catalogs have different key counts"`. This mirrors
[`nrecords()`](nrecords.md)'s own uniform-only requirement, but is a distinct check —
catalogs can agree on record count while disagreeing on key count, or vice versa.
```

## Parameters

`None`

## Returns

`std::int64_t`
    The (uniform) number of keys per catalog.

## Examples

Example 1. Every catalog has the same key count.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Ho", "Koforidua"})),
});
auto archive = pluss::kindle::Archive({c0, c1});

std::cout << archive.nkeys();
```

### Output

```text
2
```

Example 2. Catalogs with different key counts throw instead of silently picking one.

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

try {
  ragged.nkeys();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Archive::nkeys: catalogs have different key counts
```

## Notes

- Requires uniform key count across every catalog — see the important callout above.
- Requires a defined archive; throws `"Archive: undefined handle"` on a default-constructed
  `Archive()`.

## See Also

- [`Archive::minkeys`](minkeys.md) / [`Archive::maxkeys`](maxkeys.md): tolerate a ragged
  archive by reporting the range instead of throwing.
- [`Archive::nrecords`](nrecords.md): the equivalent uniform-only check for record count.
- [`Archive::shape`](shape.md): includes this method's value as its third element.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="notnull.html">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>
