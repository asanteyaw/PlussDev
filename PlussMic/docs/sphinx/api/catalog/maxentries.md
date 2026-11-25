<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2e6ff46e1add -->

# Catalog::maxentries

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
std::int64_t Catalog::maxentries() const
```

## What It Does

Returns the entry count of the **longest** key. It is the number of rows [`tabelize()`](tabelize.md) pads a ragged catalog to, and the first element of [`shape()`](shape.md). On an aligned catalog it equals [`nentries()`](nentries.md).

A catalog with no keys has a maximum of zero.

## Parameters

None.

## Returns

`std::int64_t`
    The largest key length; `0` for a catalog with no keys.

## Examples

Example 1. A ragged catalog: the longest key sets the maximum.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
    "flag"_k = pluss::kindle::Stream(pluss::make_series({1.0, 0.0})),
});

std::cout << ct.maxentries();
```

### Output

```text
5
```

Example 2. `maxentries()` is the row count `tabelize()` pads to.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto padded = ct.tabelize();
std::cout << "max=" << ct.maxentries() << " rows=" << padded.ncases();
```

### Output

```text
max=5 rows=5
```

Example 3. A catalog with no keys has a maximum of zero.

```cpp
using namespace pluss::kindle::literals;
pluss::kindle::Catalog ct(std::initializer_list<pluss::kindle::NamedStream>{});

std::cout << ct.maxentries();
```

### Output

```text
0
```

Example 4. An undefined catalog has no handle to measure.

```cpp
using namespace pluss::kindle::literals;
pluss::kindle::Catalog ct;

try {
    std::cout << ct.maxentries();
} catch (const pluss::Error& e) {
    std::cout << e.message();
}
```

### Output

```text
Catalog: undefined handle
```


## Notes

- Never throws on a ragged catalog, unlike [`nentries()`](nentries.md).
- Throws `"Catalog: undefined handle"` on a default-constructed catalog.

## See Also

- [`Catalog::minentries`](minentries.md): the shortest key.
- [`Catalog::nentries`](nentries.md): the shared count, for aligned catalogs only.
- [`Catalog::tabelize`](tabelize.md): pad a ragged catalog into a DataFrame with `maxentries()` rows.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="max.html">
    <span class="label">Previous</span>
    <span class="title">max</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
