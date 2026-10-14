<!-- pluss-hand-authored -->

# Locus::drop

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::drop(std::initializer_list<std::string_view> names) const -> Locus
Locus::drop(const std::vector<std::string>& names) const -> Locus
```

## What It Does

Removes the named catalogs, keeping every other catalog in its existing
order. Any declared link, table metadata entry, or saved join plan that
touches a dropped catalog is removed along with it, so the result never
references a catalog that no longer exists.

## Parameters

| Parameter | Meaning |
|---|---|
| `names` | The catalogs to remove. |

## Returns

A new `Locus` without the named catalogs, and without anything that
referenced them.

## Examples

Example 1. Dropping several catalogs at once.

```cpp
const auto linked = make_linked_shop();
const auto fewer = linked.drop({"items", "orders"});
print_keys(fewer);
```

### Output

```text
keys=[customers]
```

Example 2. Dropping a catalog removes the link that touched it too --
`items` was linked to `orders`; after dropping it, one relationship remains
(`orders->customers`), not two.

```cpp
std::cout << "nrelationships after dropping items=" << linked.drop({"items"}).nrelationships() << "\n";
```

### Output

```text
nrelationships after dropping items=1
```

## See Also

- [`without`](without.md): the single-catalog form of the same idea.
- [`select`](select.md): keep named catalogs instead of removing them.
