<!-- pluss-hand-authored -->

# Locus::defined, Locus::unique, Locus::empty, Locus::ntables, Locus::nrelationships, Locus::shape

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::defined() const noexcept -> bool
Locus::unique() const noexcept -> bool
Locus::empty() const -> bool
Locus::ntables() const -> std::int64_t
Locus::nrelationships() const -> std::int64_t
Locus::shape() const -> std::pair<std::int64_t, std::int64_t>
```

## What It Does

Six read-only probes of a `Locus`'s own shape, none of which touch alignment or the
cached `Nexus`. `defined()` is `false` only for a default-constructed `Locus`; every
other member here (and almost every other `Locus` method) requires a defined `Locus`
and throws otherwise. `unique()` reports whether every catalog name is distinct --
construction itself already refuses a duplicate name, so a successfully built `Locus`
is always `unique()`. `empty()` is `ntables() == 0`. `ntables()` and
`nrelationships()` count the workspace's catalogs and declared links; `shape()`
returns both as one pair, `{tables, relationships}`.

## Parameters

None.

## Returns

`defined`, `unique`, and `empty` return `bool`. `ntables` and `nrelationships` return
`std::int64_t`. `shape` returns `std::pair<std::int64_t, std::int64_t>`.

## Errors

None of these throw, including on a default (undefined) `Locus` -- `defined()` itself
is how you find out, safely, before calling anything that would throw.

## Examples

Example 1. A two-table `Locus`, before and after declaring one link.

```cpp
Catalog customers(Streams{{"id", nums({1.0, 2.0, 3.0})}, {"name", words({"Ada", "Ben", "Cy"})}});
Catalog orders(Streams{{"order_id", nums({10.0, 11.0, 12.0})}, {"cust_id", nums({1.0, 1.0, 2.0})}});
Locus shop({{"customers", customers}, {"orders", orders}});

std::cout << "defined: " << shop.defined() << '\n';
std::cout << "empty: " << shop.empty() << '\n';
std::cout << "ntables: " << shop.ntables() << '\n';
std::cout << "nrelationships (none declared yet): " << shop.nrelationships() << '\n';
const auto shape = shop.shape();
std::cout << "shape: {" << shape.first << ", " << shape.second << "}\n";
std::cout << "unique: " << shop.unique() << '\n';

const auto linked = shop.link("orders", "cust_id", "customers", "id");
std::cout << "linked nrelationships: " << linked.nrelationships() << '\n';
std::cout << "linked shape: {" << linked.shape().first << ", " << linked.shape().second << "}\n";
```

### Output

```text
defined: true
empty: false
ntables: 2
nrelationships (none declared yet): 0
shape: {2, 0}
unique: true
linked nrelationships: 1
linked shape: {2, 1}
```

`link()` returns a new `Locus` ([`link`](link.md)'s own page) -- `shop` itself still
has zero relationships after `linked` is built; `shape()`'s second field is the one
that changes.

## See Also

- [`construction`](construction.md): building the `Locus` these methods describe.
- [`link`](link.md): declaring the relationships `nrelationships`/`shape` count.
