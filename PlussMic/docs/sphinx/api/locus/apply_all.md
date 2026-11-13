<!-- pluss-hand-authored -->

# Locus::apply_all

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus Locus::apply_all(const CatalogFn& fn) const
```

## What It Does

Runs [`apply`](apply.md)'s transform over *every* catalog in the workspace,
not just one -- `fn` is called once per catalog, each time with that
catalog's current data, and the result replaces it. Links, table metadata,
and saved plans are untouched; only the catalogs' own data changes.

## Parameters

| Parameter | Meaning |
|---|---|
| `fn` | A callable from each catalog's current data to its replacement. |

## Returns

A new `Locus` with every catalog transformed.

## Examples

Example 1. Trimming every catalog to its first row at once.

```cpp
const auto linked = make_linked_shop();
const auto all_trimmed = linked.apply_all([](const Catalog& c) { return c.head(1); });
std::cout << "items rows=" << all_trimmed.catalog("items").maxrows()
          << " orders rows=" << all_trimmed.catalog("orders").maxrows() << "\n";
```

### Output

```text
items rows=1 orders rows=1
```

Both `items` (originally 5 rows) and `orders` (originally 4) are trimmed to
1 -- the same `fn` applied to each, independent of that catalog's own shape.

## See Also

- [`apply`](apply.md): the same transform, scoped to one named catalog.
- [`flow`](flow.md): running an arbitrary function over the whole `Locus`,
  not just each catalog's data.
