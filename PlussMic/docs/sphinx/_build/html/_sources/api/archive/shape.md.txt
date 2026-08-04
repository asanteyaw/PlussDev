<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 607d1b4c7311 -->

# Archive::shape

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::shape() const -> std::tuple<std::int64_t, std::int64_t, std::int64_t>
```

## What It Does

Returns `(depth(), maxrecords(), maxkeys())` as a 3-tuple — depth first, then record count,
then key count.

```{important} shape() never throws on a ragged archive — it silently uses the max, not nrecords()/nkeys()
Calling `nrecords()`/`nkeys()` directly throws when catalogs disagree on record/key count
(see [`nrecords()`](nrecords.md)/[`nkeys()`](nkeys.md)). `shape()` does **not** throw in
that situation — it silently reports [`maxrecords()`](maxrecords.md) and
[`maxkeys()`](maxkeys.md) instead, with nothing in the tuple to signal that the archive was
actually ragged. Don't treat `shape()`'s second and third elements as proof the archive is
uniform — call [`nrecords()`](nrecords.md)/[`nkeys()`](nkeys.md) directly (in a `try` block,
or after confirming with [`minrecords()`](minrecords.md)/[`minkeys()`](minkeys.md)) if you
need to know that for certain.
```

## Parameters

`None`

## Returns

`std::tuple<std::int64_t, std::int64_t, std::int64_t>`
    `(depth(), maxrecords(), maxkeys())`.

## Examples

Example 1. A uniform archive: all three values agree with their direct counterparts.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({4.0, 5.0, 6.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Ho", "Sunyani", "Koforidua"})),
});
auto archive = pluss::kindle::Archive({c0, c1});

auto [depth, records, keys] = archive.shape();
std::cout << "(" << depth << ", " << records << ", " << keys << ")";
```

### Output

```text
(2, 3, 2)
```

Example 2. A ragged archive: `shape()` reports the *maximum* record and key counts,
silently — it does not throw, even though [`nrecords()`](nrecords.md) would.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})) });
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0, 5.0, 6.0})),
});
auto ragged = pluss::kindle::Archive({c0, c1});

auto [depth, records, keys] = ragged.shape();
std::cout << "shape: (" << depth << ", " << records << ", " << keys << ")\n";
std::cout << "minrecords/maxrecords: " << ragged.minrecords() << " / " << ragged.maxrecords();
```

### Output

```text
shape: (2, 4, 1)
minrecords/maxrecords: 2 / 4
```

`shape()`'s middle element (`4`) matches `maxrecords()`, not `minrecords()` — and gives no
hint that the two catalogs actually disagree (`2` vs `4` records).

## Notes

- Order is `(depth, records, keys)` — records before keys, not the other way around; easy
  to mix up if you're used to reading it as `(depth, keys, records)`.
- Never throws on a ragged archive — see the important callout above. This is the single
  most important thing to know about this method before relying on it for validation.
- Requires a defined archive; throws `"Archive: undefined handle"` on a default-constructed
  `Archive()`.

## See Also

- [`Archive::depth`](depth.md) / [`Archive::nrecords`](nrecords.md) /
  [`Archive::nkeys`](nkeys.md): the individual, uniform-only components — `nrecords()`/
  `nkeys()` throw on a ragged archive where `shape()` silently would not.
- [`Archive::maxrecords`](maxrecords.md) / [`Archive::maxkeys`](maxkeys.md): the exact
  values `shape()` reports for its second and third elements.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename.html">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
