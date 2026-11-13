<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 45aec94fc9a8 -->

# Catalog::rename

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog Catalog::rename(std::string_view from, std::string_view to) const
Catalog Catalog::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const
Catalog Catalog::rename(const std::unordered_map<std::string, std::string>& mapping) const
```

## What It Does

Renames one or more streams without touching their values. The single-pair overload
`rename(from, to)` is a convenience wrapper that builds a one-entry mapping and forwards to
the mapping overload; every overload ultimately funnels through the same
`unordered_map`-based implementation. Streams whose names aren't mentioned in the mapping
pass through unchanged.

```{important} Renaming to a name that collides with another stream throws
Whether the collision is with another *renamed* stream or with an existing, unmentioned
stream, `rename()` throws rather than silently overwriting one stream with another. It also
throws on an unknown source key, an empty target name, or (in the `initializer_list` form
specifically) a duplicate source key within the mapping itself. See Example 3.
```

## Parameters

**from** (`std::string_view`)
    The existing stream name (single-pair overload).

**to** (`std::string_view`)
    The new name for that stream (single-pair overload).

**mapping** (`std::initializer_list<std::pair<std::string_view, std::string_view>>` or
`const std::unordered_map<std::string, std::string>&`)
    Several `old_name -> new_name` pairs, applied in one call.

## Returns

`Catalog`
    A new catalog with the requested stream(s) renamed; every other stream unchanged.

## Examples

Example 1. Rename a single stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

auto out = ct.rename("id", "identifier");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [identifier, metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
identifier               | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
metric                   | Series       | Series[float64]  | 3       | [10, 20, 30]                                     
========================================================================================================================
```

Example 2. Rename several streams in one call, via an inline mapping.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k   = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.rename({{"id", "identifier"}, {"city", "location"}});
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=3
keys: [identifier, metric, location]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
identifier               | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
metric                   | Series       | Series[float64]  | 3       | [10, 20, 30]                                     
location                 | Note         | Note             | 3       | [Accra, Tema, Kumasi]                            
========================================================================================================================
```

Example 3. Renaming a stream to a name that collides with an existing, unmentioned stream
throws.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

try {
  auto out = ct.rename({{"id", "metric"}});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::rename: duplicate target key 'metric'
```

## Notes

- The single-pair overload is a convenience wrapper over the mapping overload — same
  validation rules apply.
- Streams not mentioned in the mapping pass through unchanged, keeping their original
  position.
- See the important callout above for every distinct throw condition.

## See Also

- [`Catalog::rename_`](rename_.md): the in-place, single-pair mutating counterpart.
- [`Catalog::keyaxis`](keyaxis.md): filter streams by name pattern, rather than renaming
  them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="query.html">
    <span class="label">Previous</span>
    <span class="title">query</span>
  </a>
  <a class="next-link" href="rename_.html">
    <span class="label">Next</span>
    <span class="title">rename_</span>
  </a>
</div>
