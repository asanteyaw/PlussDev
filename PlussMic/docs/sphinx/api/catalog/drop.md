<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 49f3d8402319 -->

# Catalog::drop

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::drop(std::string_view field_name) const -> Catalog
Catalog::drop(std::span<const std::string_view> field_names) const -> Catalog
Catalog::drop(std::initializer_list<std::string_view> field_names) const -> Catalog
Catalog::drop(const std::vector<std::string>& field_names) const -> Catalog
```

## What It Does

Removes the named stream(s), keeping every other stream — the complementary operation to
[`select()`](select.md), which keeps *only* the named streams. The remaining streams
preserve their original relative order.

```{important} Every requested key must exist
Like select(), drop() throws immediately if any requested name isn't present in the
catalog — it never silently ignores an unknown name.
```

## Parameters

**field_name** (`std::string_view`)
    A single stream name to remove.

**field_names** (`std::span<const std::string_view>` / `std::initializer_list<std::string_view>` /
`const std::vector<std::string>&`)
    Several stream names to remove at once.

## Returns

`Catalog`
    A new catalog with the named stream(s) removed, everything else in its original order.

## Examples

Example 1. Drop a single stream by name.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k   = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.drop("metric");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
city                     | Note         | Note             | 3       | [Accra, Tema, Kumasi]                            
========================================================================================================================
```

Example 2. Drop several streams at once, inline.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k   = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.drop({"metric", "city"});
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
========================================================================================================================
```

Example 3. Dropping an unknown key throws.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

try {
  auto out = ct.drop("zzz");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::drop: unknown key 'zzz'
```

## Notes

- Every requested name must exist — see the important callout above.
- The remaining streams keep their original relative order.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.

## See Also

- [`Catalog::select`](select.md): the complementary operation — keep *only* the named
  streams.
- [`Catalog::has`](has.md): check a name's existence before dropping it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="distinct.html">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
