<!-- pluss-hand-authored -->

# Locus::with, Locus::with_

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus Locus::with(std::string_view name, const Catalog& value) const
Locus& Locus::with_(std::string_view name, const Catalog& value)
```

## What It Does

Adds a catalog, or replaces the one already using `name`. If `name` already
exists, its data is replaced but every link, table meta entry, and saved
plan that names it stays in place -- unlike [`without`](without.md), nothing
is removed, so a link that named the old data now resolves against the new
data instead, and [`audit()`](audit.md) will re-check it the next time it's
called. `with_` is the in-place form: it mutates the receiver and returns a
reference to it, rather than returning a new `Locus`.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | The catalog to add or replace. |
| `value` | The new catalog data. |

## Returns

`with` returns a new `Locus`. `with_` returns a reference to the mutated
receiver.

## Examples

Example 1. Replacing an existing catalog leaves the original `Locus`
untouched, and appending a new name grows the workspace.

```cpp
const auto shop = make_shop();
const auto replaced = shop.with("items", shop.catalog("customers"));
std::cout << "replaced items has(name)=" << replaced.catalog("items").has("name")
          << " original items has(sku)=" << shop.catalog("items").has("sku") << "\n";
const auto appended = shop.with("extra", shop.catalog("items"));
std::cout << "appended ntables=" << appended.ntables() << "\n";
```

### Output

```text
replaced items has(name)=true original items has(sku)=true
appended ntables=4
```

`replaced`'s `items` now has the `customers` catalog's own `name` stream --
`shop` itself is unaffected, the usual immutable-handle pattern.

Example 2. `with_` mutates in place.

```cpp
auto mutated = shop;
mutated.with_("orders", shop.catalog("items"));
std::cout << "with_ changed in place: " << mutated.catalog("orders").has("sku") << "\n";
```

### Output

```text
with_ changed in place: true
```

## See Also

- [`inject`](inject.md): add several catalogs (or an `Archive`'s slices) at
  once.
- [`without`](without.md): the removal counterpart, which does drop links
  and metadata that touched the catalog.
