<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b134de772c74 -->

# Catalog::set

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::set(std::string_view key, const Stream& value) const -> Catalog
Catalog::set(std::initializer_list<std::pair<std::string_view, Stream>> items) const -> Catalog
Catalog::set(std::initializer_list<NamedStream> items) const -> Catalog
```

## What It Does

Adds a stream under a new key, or overwrites the stream already at an existing key — an
upsert, unlike [`select()`](select.md)/[`drop()`](drop.md)/[`rename()`](rename.md), which
all throw on an unknown key. A new key is appended at the end of the catalog's key order;
an existing key keeps its original position, with only its stream replaced.

```{important} Braced-list literals can be ambiguous between the two multi-item overloads
`{"key", stream}`-style braces can match *either* `std::pair<string_view, Stream>` or
`NamedStream`, so a multi-item call like `ct.set({{"a", s1}, {"b", s2}})` fails to compile
with an ambiguous-overload error. Use the `"key"_k = stream` literal syntax (which
unambiguously builds a `NamedStream`) for multi-item calls instead — see Example 3.
```

## Parameters

**key** (`std::string_view`)
    The stream name to add or overwrite (single-item overload).

**value** (`const Stream&`)
    The stream to store under `key` (single-item overload).

**items** (`std::initializer_list<std::pair<std::string_view, Stream>>` or
`std::initializer_list<NamedStream>`)
    Several `key -> stream` entries to add or overwrite in one call.

## Returns

`Catalog`
    A new catalog with the requested key(s) set to the given stream(s); every other stream
    unchanged.

## Examples

Example 1. Adding a new key appends it to the catalog.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

auto out = ct.set("flag", pluss::kindle::Stream(pluss::make_series({1.0, 0.0, 1.0})));
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, flag]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
flag                     | Series       | Series[float64]  | 3       | [1, 0, 1]                                        
========================================================================================================================
```

Example 2. Setting an existing key overwrites its stream in place — the key count and
position don't change.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

auto out = ct.set("id", pluss::kindle::Stream(pluss::make_series({9.0, 9.0, 9.0})));
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
id                       | Series       | Series[float64]  | 3       | [9, 9, 9]                                        
========================================================================================================================
```

Example 3. Set several streams at once using `"key"_k = stream` literals — the
unambiguous syntax for the multi-item overload. See the important callout above for why
plain braces don't work here.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

auto out = ct.set({
    "a"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({2.0})),
});
std::cout << out.nkeys();
```

### Output

```text
3
```

## Notes

- Upsert semantics: adds a new key or overwrites an existing one, never throws on an
  unknown key (unlike `select()`/`drop()`/`rename()`).
- New keys are appended at the end; existing keys keep their position.
- Prefer `"key"_k = stream` literals for multi-item calls — see the important callout
  above.

## See Also

- [`Catalog::set_`](set_.md): the in-place mutating counterpart.
- [`Catalog::drop`](drop.md): remove a stream, the inverse of adding one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="set_.html">
    <span class="label">Next</span>
    <span class="title">set_</span>
  </a>
</div>
