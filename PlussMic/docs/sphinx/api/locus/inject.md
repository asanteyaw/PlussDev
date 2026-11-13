<!-- pluss-hand-authored -->

# Locus::inject, Locus::inject_

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus Locus::inject(std::string_view name, const Catalog& value) const
Locus& Locus::inject_(std::string_view name, const Catalog& value)
Locus Locus::inject(std::initializer_list<std::pair<std::string_view, Catalog>> catalogs) const
Locus& Locus::inject_(std::initializer_list<std::pair<std::string_view, Catalog>> catalogs)
Locus Locus::inject(const Archive& archive) const
Locus& Locus::inject_(const Archive& archive)
```

## What It Does

Adds catalogs to the workspace -- one at a time, as a batch of
`(name, Catalog)` pairs, or as every depth slice of an `Archive` (the same
construction [`Locus::from_archive`](construction.md) uses, added to an
*existing* workspace instead of building a new one). The single-catalog
overload behaves exactly like [`with`](with.md) -- the two names are
interchangeable spellings for the same operation. `inject_` is the in-place
form, mutating the receiver.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | A catalog to add or replace. |
| `value` | The catalog data. |
| `catalogs` | Several `(name, Catalog)` pairs, added in order. |
| `archive` | An `Archive`; each depth slice becomes a catalog, named by its label when the label is text, otherwise `"catalog_<i>"`. |

## Returns

`inject` returns a new `Locus`. `inject_` returns a reference to the mutated
receiver.

## Examples

Example 1. A batch of two catalogs at once.

```cpp
const auto shop = make_shop();
const auto injected = shop.inject({{"extra", shop.catalog("items")}, {"more", shop.catalog("orders")}});
std::cout << "injected ntables=" << injected.ntables() << "\n";
```

### Output

```text
injected ntables=5
```

Example 2. Injecting an `Archive`'s labeled slices.

```cpp
const pluss::kindle::Archive archive(std::vector<Catalog>{shop.catalog("orders"), shop.catalog("items")},
                                     words({"o", "i"}));
const auto injected_archive = shop.inject(archive);
std::cout << "injected archive ntables=" << injected_archive.ntables() << "\n";
```

### Output

```text
injected archive ntables=5
```

Two slices, named `o` and `i` from the archive's own text labels, added to
the existing three catalogs.

## See Also

- [`with`](with.md): the exact same single-catalog operation, under its
  other name.
- [`construction`](construction.md): `from_archive`, the same slice-to-catalog
  rule applied when *building* a `Locus` instead of adding to one.
