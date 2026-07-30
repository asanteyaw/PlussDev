<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eb36c4f3fa25 -->

# Catalog::select

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::select(std::span<const std::string_view> field_names) const -> Catalog
Catalog::select(std::initializer_list<std::string_view> field_names) const -> Catalog
Catalog::select(const std::vector<std::string>& field_names) const -> Catalog
```

## What It Does

Keeps only the named streams, in the order requested — the direct counterpart to
[`drop()`](drop.md), which keeps everything *except* the named streams. Every overload
funnels through the same `std::vector<std::string>` implementation; pick whichever
container is most convenient at the call site.

```{important} Every requested key must exist
Unlike keyaxis()'s name/kind filters, select() does not silently skip unknown names — it
throws immediately if any requested key isn't present in the catalog. Check with
[`has()`](has.md) first if a name might be missing.
```

## Parameters

**field_names** (`std::span<const std::string_view>` / `std::initializer_list<std::string_view>` /
`const std::vector<std::string>&`)
    The stream names to keep, in the order they should appear in the result.

## Returns

`Catalog`
    A new catalog containing only the requested streams, in the requested order.

## Examples

Example 1. Select two of three streams, inline.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k   = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.select({"id", "city"});
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

Example 2. An unknown name throws immediately, before building any result.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

try {
  auto out = ct.select({"zzz"});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::select: unknown key 'zzz'
```

## Notes

- Every requested name must exist — see the important callout above.
- Result order follows `field_names`' order, not the source catalog's key order.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.

## See Also

- [`Catalog::drop`](drop.md): the complementary operation — keep everything *except* the
  named streams.
- [`Catalog::keys`](keys.md): select a subset of streams for further per-stream
  transformation, rather than a standalone projection.
- [`Catalog::has`](has.md): check a name's existence before selecting it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_txt.html">
    <span class="label">Previous</span>
    <span class="title">save_txt</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
