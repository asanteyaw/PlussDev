<!-- pluss-hand-authored -->

# CatalogGroupBuilder::by

[Back to CatalogGroupBuilder API Reference](../../catalog_group_accessor_api_reference)

## Signature

```cpp
std::unordered_map<std::string, Catalog> CatalogGroupBuilder::by(std::string_view key_field) const
```

## What It Does

Splits a catalog by the values of one stream. `Catalog::group()` only binds the catalog; `by` reads the stream named
`key_field` as the grouping key and returns one complete sub-catalog for every distinct key value. Each sub-catalog keeps
every stream, including the key stream itself, restricted to the rows of that group.

## Parameters

**key_field** (`std::string_view`)

The name of the stream to group by.

## Returns

`std::unordered_map<std::string, Catalog>`
    One catalog per distinct key value, keyed by the value written as text. The map is unordered, so look groups up by
    key (`groups.at("east")`) and do not rely on iteration order.

## Errors

- A name that is not a stream of the catalog is refused with `Catalog::group().by: unknown key '<name>'`.
- An empty name is refused with `Catalog::group().by: key field cannot be empty`.

## Examples

Example 1. Split a catalog into one sub-catalog per value of a text stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "region"_k = pluss::kindle::Stream(pluss::make_note({"east", "west", "east", "west"})),
    "sales"_k  = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0})),
});

auto groups = ct.group().by("region");
std::cout << "groups: " << groups.size() << "\n";
std::cout << groups.at("east");
```

### Output

```text
groups: 2
========================================================================================================================
kindle::Catalog  keys=2
keys: [region, sales]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
region                   | Note         | Note             | 2       | [east, east]                                     
sales                    | Series       | Series[float64]  | 2       | [10, 30]                                         
========================================================================================================================
```

Example 2. The other group.

```cpp
std::cout << groups.at("west");
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [region, sales]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
region                   | Note         | Note             | 2       | [west, west]                                     
sales                    | Series       | Series[float64]  | 2       | [20, 40]                                         
========================================================================================================================
```

Example 3. An unknown key is refused.

```cpp
try {
  auto out = ct.group().by("nope");
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
Catalog::group().by: unknown key 'nope'
```

## Notes

- Every stream of the catalog is carried into each sub-catalog, so the grouping key can still be read there.

## See Also

- [`Catalog::group`](../catalog/group.md): enters the builder.

<div class="pluss-inline-prev-next">
</div>
