<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e3fa4bfc478f -->

# Catalog::keys

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::keys() const -> const std::vector<std::string>&
Catalog::keys(std::string_view key) const -> Catalog
Catalog::keys(std::span<const std::string_view> field_names) const -> Catalog
Catalog::keys(std::initializer_list<std::string_view> field_names) const -> Catalog
```

## What It Does

Two genuinely different operations share this one overloaded name:

- **`keys()`** (no arguments) returns the catalog's stream names as a plain
  `const std::vector<std::string>&` — a read-only view, in the catalog's own key order.
- **`keys(key)`** / **`keys(field_names)`** projects the catalog down to the named
  streams and returns that smaller `Catalog` directly.

```{important} keys(names) is now direct projection
`ct.keys({"score", "city"})` returns a smaller catalog containing exactly those streams, in
the requested order. Chained operations then run on that projected catalog.
```

## Parameters

**key** (`std::string_view`)
    A single stream name to select.

**field_names** (`std::span<const std::string_view>` or
`std::initializer_list<std::string_view>`)
    Several stream names to select at once.

## Returns

`const std::vector<std::string>&` (no-argument overload)
    A read-only reference to the catalog's stream names, in key order.

`Catalog` (name-argument overloads)
    A smaller catalog containing exactly the requested streams, in the requested order.

## Examples

Example 1. The no-argument overload returns every stream name, in order.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k   = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

for (const auto& name : ct.keys()) {
  std::cout << name << " ";
}
```

### Output

```text
id metric city 
```

Example 2. `keys(key)` projects to one stream, and the result behaves like any other
catalog.

```cpp
auto ct = pluss::kindle::Catalog({
    "score"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.keys("score").fillnull(0.0);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [score]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
score                    | Series       | Series[float64]  | 3       | [1, 0, 3]                                        
========================================================================================================================
```

Example 3. `keys({...})` with an `initializer_list` selects several streams at once.

```cpp
auto ct = pluss::kindle::Catalog({
    "score"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
    "id"_k = pluss::kindle::Stream(pluss::make_series({7.0, 8.0, 9.0})),
});

auto out = ct.keys({"city", "score"});
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [city, score]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
city                     | Note         | Note             | 3       | [Accra, Tema, Kumasi]                            
score                    | Series       | Series[float64]  | 3       | [1, NULL, 3]                                     
========================================================================================================================
```

## Notes

- The no-argument overload returns a reference into the catalog's own storage — no copy.
- The name-argument overloads return a projected `Catalog`, not an accessor shell.
- Selecting an unknown key name throws — see [`has()`](has.md) to check first.

## See Also

- [`Catalog::has`](has.md): check whether a key exists before selecting it.
- [`Catalog::drop`](drop.md): remove names instead of projecting them.
- [`Catalog::at`](at.md) / [`Catalog::get`](get.md): fetch a single stream directly as a
  `Stream` instead of keeping it inside a catalog.
- [`Catalog::select`](select.md): static row-wise conditional choose between whole
  catalogs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keyaxis.html">
    <span class="label">Previous</span>
    <span class="title">keyaxis</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
