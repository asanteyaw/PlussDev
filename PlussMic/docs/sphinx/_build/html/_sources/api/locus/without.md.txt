<!-- pluss-hand-authored -->

# Locus::without, Locus::without_

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::without(std::string_view name) const -> Locus
Locus& Locus::without_(std::string_view name)
```

## What It Does

Removes one catalog, together with every link, table metadata entry, and
saved join plan that touches it -- the single-name counterpart of
[`drop`](drop.md). `without_` is the in-place form.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | The catalog to remove. |

## Returns

`without` returns a new `Locus`. `without_` returns a reference to the
mutated receiver.

## Errors

Throws on an unknown catalog name.

## Examples

Example 1. Removing `items` from a linked workspace removes the link
touching it (`items->orders`), leaving `orders->customers` alone.

```cpp
const auto linked = make_linked_shop();
const auto fewer = linked.without("items");
std::cout << "without ntables=" << fewer.ntables() << " nrelationships=" << fewer.nrelationships() << "\n";
std::cout << "original ntables=" << linked.ntables() << " nrelationships=" << linked.nrelationships() << "\n";
```

### Output

```text
without ntables=2 nrelationships=1
original ntables=3 nrelationships=2
```

Example 2. An unknown name is refused.

```cpp
print_error("unknown without", [&] { (void)linked.without("nope"); });
```

### Output

```text
unknown without: Locus::without: unknown catalog 'nope'
```

## See Also

- [`drop`](drop.md): the multi-name form of the same operation.
- [`with`](with.md): adding or replacing a catalog, the inverse direction.
