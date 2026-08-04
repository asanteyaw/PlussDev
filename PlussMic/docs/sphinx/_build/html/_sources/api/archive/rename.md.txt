<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 622929f9f204 -->

# Archive::rename

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
Archive::rename(std::string_view from, std::string_view to) const -> Archive
Archive::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const -> Archive
```

## What It Does

Renames one or more streams, applied to every catalog — the single-pair overload for
renaming exactly one key, the initializer-list overload for renaming several at once in one
call. Delegates to each catalog's own `Catalog::rename()`, so a `from` name missing from
some catalog throws that catalog's own `"Catalog::rename: unknown key '<name>'"`. Only
names change; payload values, order, and depth labels are untouched.

## Parameters

**from** (`std::string_view`, single-pair overload)
    The existing stream name to rename.

**to** (`std::string_view`, single-pair overload)
    The new name.

**mapping** (`std::initializer_list<std::pair<std::string_view, std::string_view>>`,
multi-pair overload)
    One or more `{from, to}` pairs to rename in a single call.

## Returns

`Archive`
    A new archive, same depth, depth labels, and payload values as the receiver, with the
    named stream(s) renamed in every catalog.

## Examples

Example 1. Rename a single stream.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra"})),
});
auto archive = pluss::kindle::Archive({c0}, pluss::kindle::Stream(pluss::make_note({"train"})));

auto out = archive.rename("id", "user_id");
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [user_id, city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
user_id                  | Series       | Series[float64]  | 1       | [1]                                              
city                     | Note         | Note             | 1       | [Accra]                                          
========================================================================================================================
```

Example 2. Rename several streams in one call.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.rename({{"id", "user_id"}, {"city", "location"}});
std::cout << out.catalog(0);
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [user_id, location]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
user_id                  | Series       | Series[float64]  | 1       | [1]                                              
location                 | Note         | Note             | 1       | [Accra]                                          
========================================================================================================================
```

Example 3. Renaming a key missing from one catalog throws — a ragged-archive gotcha
shared with [`keys(...)`](keys.md)/[`drop()`](drop.md).

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra"})),
});
auto c1 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({2.0})) });
auto ragged = pluss::kindle::Archive({c0, c1});

try {
  ragged.rename("city", "location");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::rename: unknown key 'city'
```

## Notes

- Only stream names change; payload values, row order, and depth labels are unaffected.
- Throws on any catalog missing a `from` key — same ragged-archive requirement as
  [`keys(...)`](keys.md)/[`drop()`](drop.md).

## See Also

- [`Archive::keys`](keys.md) / [`Archive::drop`](drop.md): the other key/column
  operations sharing the same "every catalog must have the key" requirement.
- [`Archive::has`](has.md): confirm a key exists in every catalog before renaming it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nrecords.html">
    <span class="label">Previous</span>
    <span class="title">nrecords</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>
