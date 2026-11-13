<!-- pluss-hand-authored -->

# Locus::keys, Locus::catalogs, Locus::has, Locus::catalog, Locus::get

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
const std::vector<std::string>& Locus::keys() const
const std::vector<Catalog>& Locus::catalogs() const
bool Locus::has(std::string_view name) const
Catalog Locus::catalog(std::string_view name) const
Catalog Locus::catalog(std::int64_t index) const
std::optional<Catalog> Locus::get(std::string_view name) const
```

## What It Does

`keys()` and `catalogs()` return the workspace's table names and member catalogs, in
construction order, as parallel vectors. `has()` checks whether a name exists.
`catalog()` looks one member up, by name or by index -- a negative index counts from
the end (`-1` is the last table), matching `Catalog`'s and `DataFrame`'s own indexing
convention -- and throws on a name or index that doesn't exist. `get()` is the
non-throwing equivalent of the by-name overload, returning `std::nullopt` instead of
throwing.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | A table name, for `has`, `catalog`, and `get`. |
| `index` | A table position; negative counts from the end. |

## Returns

`keys()` and `catalogs()` return references into the `Locus`'s own storage. `has`
returns `bool`. `catalog` returns a `Catalog` by value (a cheap handle copy, not a
deep copy). `get` returns `std::optional<Catalog>`.

## Errors

`catalog(name)` throws on an unknown name; `catalog(index)` throws on an
out-of-range index. `get` never throws -- an unknown name is `std::nullopt`.

## Examples

Example 1. Reading members back by name and by index, including the not-found paths.

```cpp
Catalog customers(Streams{{"id", nums({1.0, 2.0, 3.0})}, {"name", words({"Ada", "Ben", "Cy"})}});
Catalog orders(Streams{{"order_id", nums({10.0, 11.0, 12.0})}, {"cust_id", nums({1.0, 1.0, 2.0})}});
Catalog items(Streams{{"sku", words({"a", "b"})}});
Locus shop({{"customers", customers}, {"orders", orders}, {"items", items}});

std::cout << "keys: ";
for (const auto& n : shop.keys()) std::cout << n << ' ';
std::cout << '\n';

std::cout << "has(orders): " << shop.has("orders") << '\n';
std::cout << "has(nope): " << shop.has("nope") << '\n';

std::cout << "catalog(\"orders\").nkeys(): " << shop.catalog("orders").nkeys() << '\n';
std::cout << "catalog(0).has(id): " << shop.catalog(0).has("id") << '\n';
std::cout << "catalog(-1).has(sku): " << shop.catalog(-1).has("sku") << '\n';

std::cout << "get(\"items\") has value: " << shop.get("items").has_value() << '\n';
std::cout << "get(\"nope\") has value: " << shop.get("nope").has_value() << '\n';

try {
  (void)shop.catalog("nope");
  std::cout << "catalog(\"nope\"): did not throw\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << "catalog(\"nope\") threw: " << msg.substr(0, msg.find('\n')) << '\n';
}
try {
  (void)shop.catalog(99);
  std::cout << "catalog(99): did not throw\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << "catalog(99) threw: " << msg.substr(0, msg.find('\n')) << '\n';
}

std::cout << "catalogs().size(): " << shop.catalogs().size() << '\n';
```

### Output

```text
keys: customers orders items 
has(orders): true
has(nope): false
catalog("orders").nkeys(): 2
catalog(0).has(id): true
catalog(-1).has(sku): true
get("items") has value: true
get("nope") has value: false
catalog("nope") threw: Locus::catalog: unknown catalog 'nope'
catalog(99) threw: Locus::catalog: index 99 out of range for 3 tables
catalogs().size(): 3
```

`catalog(-1)` reaches `items`, the last table, the same as `catalog(2)` would -- `-1`
counts from the end rather than meaning "invalid."

## See Also

- [`state`](state.md): `ntables()`, the count these accessors index into.
- [`link`](link.md): `neighbors`/`path`, which answer questions about a table by name
  the same way `has`/`catalog` do.
