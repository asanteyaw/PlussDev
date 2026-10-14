<!-- pluss-hand-authored -->

# Locus::head, Locus::tail

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::head(std::int64_t n = 5) const -> Locus
Locus::tail(std::int64_t n = 5) const -> Locus
```

## What It Does

Keeps the first (`head`) or last (`tail`) `n` catalogs, in construction/
`keys()` order -- a workspace-level slice, not a per-catalog row slice
(`Catalog::head`/`Catalog::tail` do that). `n` larger than `ntables()` is not
an error; it just keeps everything.

## Parameters

| Parameter | Meaning |
|---|---|
| `n` | How many catalogs to keep, from the front (`head`) or the back (`tail`). Default `5`. |

## Returns

A new `Locus` with the selected catalogs, same relative order.

## Examples

Example 1. A three-table workspace (`customers`, `orders`, `items`), head
and tail.

```cpp
const auto shop = make_shop();
print_keys(shop.head(2));
print_keys(shop.tail(1));
print_keys(shop.head(10));
```

### Output

```text
keys=[customers, orders]
keys=[items]
keys=[customers, orders, items]
```

`head(10)` on a 3-table workspace keeps all three -- `n` past the end is not
refused.

## See Also

- [`select`](select.md): keep catalogs by name instead of position.
- [`drop`](drop.md): remove catalogs by name instead of keeping a slice.
