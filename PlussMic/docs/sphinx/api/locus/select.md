<!-- pluss-hand-authored -->

# Locus::select

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::select(std::initializer_list<std::string_view> names) const -> Locus
Locus::select(const std::vector<std::string>& names) const -> Locus
```

## What It Does

Keeps only the named catalogs, in the order `names` lists them -- unlike
[`head`/`tail`](head.md), `select` can also reorder. Every link,
table meta entry, and saved plan touching a *kept* catalog carries over
unchanged; one touching a *dropped* catalog is removed, the same as
[`drop`](drop.md).

## Parameters

| Parameter | Meaning |
|---|---|
| `names` | The catalogs to keep, in the order to keep them. |

## Returns

A new `Locus` with exactly the named catalogs, in `names`' order.

## Errors

Throws on an unknown catalog name.

## Examples

Example 1. Reordering as a side effect of selecting.

```cpp
const auto shop = make_shop();
print_keys(shop.select({"items", "customers"}));
```

### Output

```text
keys=[items, customers]
```

Example 2. An unknown name is refused.

```cpp
print_error("unknown select", [&] { (void)shop.select({"customers", "nope"}); });
```

### Output

```text
unknown select: Locus::select: unknown catalog 'nope'
```

## See Also

- [`drop`](drop.md): the complementary operation -- remove named catalogs
  instead of keeping them.
- [`head`](head.md): keep a slice by position instead of by name.
