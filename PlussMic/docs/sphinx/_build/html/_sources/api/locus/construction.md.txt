<!-- pluss-hand-authored -->

# Locus::from_archive, Locus::from_nexus

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus(Catalogs catalogs) -> Locus
Locus(std::initializer_list<std::pair<std::string, Catalog>> catalogs) -> Locus

static Locus::from_archive(const Archive& archive) -> Locus
static Locus::from_nexus(const Nexus& nexus) -> Locus
```

## What It Does

The ordinary constructor takes a vector or brace list of `(name, Catalog)` pairs and
builds a workspace with no relationships yet -- `Locus({{"customers", customers},
{"orders", orders}})`. A default-constructed `Locus` is undefined, the same
handle-semantics convention [`Nexus`](../../nexus_api_reference.md) and `Catalog`
follow; every member catalog must itself be defined, and names must be unique, or
construction throws.

`from_archive` builds one catalog per depth slice of an `Archive`, named by the
archive's own labels when they are text, otherwise `"catalog_<i>"`. `from_nexus` is
the bridge from the `DataFrame` side of `bundle`: every frame in the `Nexus` becomes a
`Catalog` (via `kindelize()`), and relationships, table metadata, and saved plans all
carry over unchanged.

## Parameters

| Parameter | Meaning |
|---|---|
| `catalogs` | The workspace's members, as `(name, Catalog)` pairs. |
| `archive` | An `Archive`; one `Locus` catalog per depth slice. |
| `nexus` | A `Nexus` whose frames become this `Locus`'s catalogs. |

## Returns

A `Locus` with the members produced by that constructor or factory.

## Errors

The ordinary constructor throws on an undefined member catalog or a duplicate name.
`from_archive` and `from_nexus` throw on an undefined `Archive`/`Nexus`.

## Examples

Example 1. The ordinary constructor, a default (undefined) `Locus`, and both named
factories.

```cpp
Locus empty_locus;
std::cout << "default defined: " << empty_locus.defined() << '\n';
std::cout << "default empty: " << empty_locus.empty() << '\n';

Catalog customers(Streams{{"id", nums({1.0, 2.0, 3.0})}, {"name", words({"Ada", "Ben", "Cy"})}});
Catalog orders(Streams{{"order_id", nums({10.0, 11.0, 12.0})}, {"cust_id", nums({1.0, 1.0, 2.0})}});
Locus shop({{"customers", customers}, {"orders", orders}});
std::cout << "shop defined: " << shop.defined() << '\n';
std::cout << "shop ntables: " << shop.ntables() << '\n';

auto archive = pluss::kindle::Archive({customers, orders});
auto from_arch = Locus::from_archive(archive);
std::cout << "from_archive ntables: " << from_arch.ntables() << '\n';
std::cout << "from_archive names: ";
for (const auto& n : from_arch.keys()) std::cout << n << ' ';
std::cout << '\n';

pluss::bundle::Nexus nexus_ws({
    {"customers", pluss::table::DataFrame({{"id", pluss::table::Slate(pluss::make_series({1.0, 2.0}))}})},
});
auto from_nx = Locus::from_nexus(nexus_ws);
std::cout << "from_nexus ntables: " << from_nx.ntables() << '\n';
```

### Output

```text
default defined: false
default empty: true
shop defined: true
shop ntables: 2
from_archive ntables: 2
from_archive names: catalog_0 catalog_1 
from_nexus ntables: 1
```

`from_archive`'s two slices get the generated `catalog_0`/`catalog_1` names because the
source `Archive` here has no text labels. `from_nexus`'s one frame becomes one catalog,
named after the `Nexus`'s own table name.

## See Also

- [`state`](state.md): `defined`, `empty`, and the rest of a freshly built `Locus`'s
  readable state.
- [`catalogs`](catalogs.md): reading the member catalogs back out by name or index.
