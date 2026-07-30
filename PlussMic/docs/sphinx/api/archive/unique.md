<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 405866cfe223 -->

# Archive::unique

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::unique() const noexcept -> bool
```

## What It Does

Reports whether **this `Archive` handle is the sole owner** of its underlying payload — a
reference-count check on the internal `intrusive_ptr`, not a check on the archive's
contents. Same semantics as [`Stream::unique`](../stream/unique.md)/
[`Catalog::unique`](../catalog/unique.md): `Archive`'s copy constructor is a cheap handle
copy (shared ownership), not a deep copy of every catalog it holds.

```{important} unique() is about ownership, not distinct catalogs or keys
An archive whose catalogs share duplicate keys or identical data still reports
`unique(): true` if no other `Archive` handle shares its payload. This has nothing to do
with the *content* of the archive.
```

## Parameters

`None`

## Returns

`bool`
    `true` if no other `Archive` handle currently shares this archive's payload; `false` if
    at least one copy exists elsewhere.

## Examples

Example 1. A freshly constructed archive is its own sole owner.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});
auto archive = pluss::kindle::Archive({c0});

std::cout << archive.unique();
```

### Output

```text
1
```

Example 2. Copying the archive drops `unique()` to `false` on both handles while the copy
is alive.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
});
auto arc1 = pluss::kindle::Archive({c0});

auto arc2 = arc1;
std::cout << "arc1: " << arc1.unique() << "\n";
std::cout << "arc2: " << arc2.unique();
```

### Output

```text
arc1: 0
arc2: 0
```

## Notes

- Ownership-sharing check, not a content-distinctness check — see the important callout
  above.
- Moving an archive (`std::move`) does not create a second owner.
- `noexcept`; safe to call at any time.

## See Also

- [`Archive::defined`](defined.md): a different ownership-adjacent check — whether the
  handle has a payload at all, regardless of sharing.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_table.html">
    <span class="label">Previous</span>
    <span class="title">to_table</span>
  </a>
</div>
