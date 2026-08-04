<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2b75576c50b8 -->

# Archive::drop

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
Archive::drop(std::span<const std::string_view> keys) const -> Archive
Archive::drop(std::initializer_list<std::string_view> keys) const -> Archive
Archive::drop(const std::vector<std::string>& keys) const -> Archive
```

## What It Does

Returns a new archive with the named streams removed, applied to every catalog — the
inverse of projecting keys with [`keys(...)`](keys.md). Delegates to each
catalog's own `Catalog::drop()`, so (perhaps surprisingly) a `keys` entry that doesn't
exist in some catalog still throws that catalog's own `"Catalog::drop: unknown key
'<name>'"` rather than silently skipping it. Depth labels (if set) are preserved on the
result.

```{important} Dropping an already-absent key throws — this is not a tolerant no-op
Unlike some "drop if present" APIs, `Archive::drop()` requires every named key to actually
exist in every catalog. On a ragged archive, drop a key that's already missing from one
catalog and the whole call throws — check with [`has()`](has.md) first if you're not sure.
```

## Parameters

**keys** (`std::span<const std::string_view>`, `std::initializer_list<std::string_view>`,
or `const std::vector<std::string>&`)
    The stream names to remove, applied uniformly to every catalog.

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with the named streams
    removed from every catalog.

## Examples

Example 1. Drop a stream from every catalog.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra"})),
});
auto archive = pluss::kindle::Archive({c0}, pluss::kindle::Stream(pluss::make_note({"train"})));

auto out = archive.drop({"city"});
std::cout << "depth labels preserved: " << out.label().has() << "\n";
std::cout << out.catalog(0);
```

### Output

```text
depth labels preserved: 1
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [1]                                              
========================================================================================================================
```

Example 2. Dropping a key that's already missing from one catalog throws — see the
important callout above.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra"})),
});
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto ragged = pluss::kindle::Archive({c0, c1});

try {
  ragged.drop({"city"});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::drop: unknown key 'city'
```

## Notes

- No no-argument overload — `keys` must be supplied explicitly.
- Throws on any catalog missing a key you're trying to drop — see the important callout
  above.
- Depth labels are preserved on the result.

## See Also

- [`Archive::keys`](keys.md): keep named streams instead of removing them.
- [`Archive::has`](has.md): confirm a key exists in every catalog before dropping it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dloc.html">
    <span class="label">Previous</span>
    <span class="title">dloc</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
