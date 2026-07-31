<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 675ae73ae8dc -->

# Archive::select

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
Archive::select(std::span<const std::string_view> keys) const -> Archive
Archive::select(std::initializer_list<std::string_view> keys) const -> Archive
Archive::select(const std::vector<std::string>& keys) const -> Archive
```

## What It Does

Returns a new archive keeping only the named streams, applied to every catalog. Delegates
to each catalog's own `Catalog::select()`, so a `keys` entry that doesn't exist in some
catalog throws that catalog's own `"Catalog::select: unknown key '<name>'"` — same as
calling [`has()`](has.md) would reveal in advance for a ragged archive. Depth labels (if
set) are preserved on the result.

## Parameters

**keys** (`std::span<const std::string_view>`, `std::initializer_list<std::string_view>`,
or `const std::vector<std::string>&`)
    The stream names to keep, applied uniformly to every catalog.

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with every catalog reduced
    to just the named streams.

## Examples

Example 1. Select a subset of streams with an inline list.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra"})),
    "score"_k = pluss::kindle::Stream(pluss::make_series({9.5})),
});
auto archive = pluss::kindle::Archive({c0}, pluss::kindle::Stream(pluss::make_note({"train"})));

auto out = archive.select({"id", "score"});
std::cout << "depth labels preserved: " << out.label().has() << "\n";
std::cout << out.catalog(0);
```

### Output

```text
depth labels preserved: 1
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, score]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 1       | [1]                                              
score                    | Series       | Series[float64]  | 1       | [9.5]                                            
========================================================================================================================
```

Example 2. Selecting a key missing from at least one catalog throws — a common outcome on
a ragged archive.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra"})),
});
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto ragged = pluss::kindle::Archive({c0, c1});

try {
  ragged.select({"city"});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::select: unknown key 'city'
```

## Notes

- No no-argument overload — `keys` must be supplied explicitly.
- Throws on any catalog missing a requested key; check with [`has()`](has.md) first on a
  potentially ragged archive.
- Depth labels are preserved on the result.

## See Also

- [`Archive::drop`](drop.md): the inverse — remove named streams instead of keeping them.
- [`Archive::has`](has.md): confirm a key exists in every catalog before selecting it.
- [`Archive::keyaxis`](keyaxis.md): select by substring/regex/kind instead of an explicit
  name list.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename.html">
    <span class="label">Previous</span>
    <span class="title">rename</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>
