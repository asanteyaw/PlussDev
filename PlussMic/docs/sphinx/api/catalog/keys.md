<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3d8e652e477d -->

# Catalog::keys

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
const std::vector<std::string>& Catalog::keys() const
Stream Catalog::keys(std::string_view key) const
Catalog Catalog::keys(std::span<const std::string_view> field_names) const
Catalog Catalog::keys(std::initializer_list<std::string_view> field_names) const
```

## What It Does

Three genuinely different operations share this one overloaded name:

- **`keys()`** (no arguments) returns the catalog's stream names as a plain
  `const std::vector<std::string>&` — a read-only view, in the catalog's own
  key order.
- **`keys(key)`** (a single name) extracts **one stream directly** and
  returns it as a `Stream` — not a one-key catalog.
- **`keys(field_names)`** (a span or initializer list of several names)
  projects the catalog down to those streams and returns that smaller
  `Catalog`.

```{important} The single-key overload returns a Stream, not a one-key Catalog
`ct.keys("score")` returns `Stream`, so chaining continues on the `Stream`
surface (`.fillnull()`, `.nt()`, `.tl()`, ...), not the `Catalog` surface.
This is easy to get backwards from the multi-name overloads, which *do*
return a `Catalog` — pass an `initializer_list`/`span` (even of one name,
`ct.keys({"score"})`) if you specifically want a one-key `Catalog` back
instead of a bare `Stream`.
```

## Parameters

**key** (`std::string_view`)
    A single stream name to extract as a `Stream`.

**field_names** (`std::span<const std::string_view>` or
`std::initializer_list<std::string_view>`)
    Several stream names to project down to, returned as a `Catalog`.

## Returns

`const std::vector<std::string>&` (no-argument overload)
    A read-only reference to the catalog's stream names, in key order.

`Stream` (single-key overload)
    The named stream, extracted directly.

`Catalog` (multi-name overloads)
    A smaller catalog containing exactly the requested streams, in the
    requested order.

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

Example 2. `keys(key)` extracts a single stream directly — see the important
callout above.

```cpp
using namespace pluss::kindle::literals;

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
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 0                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

`.fillnull(0.0)` is a `Stream` method here — `keys("score")` handed back a
bare `Stream`, not a catalog to keep projecting.

Example 3. `keys({...})` with an `initializer_list` selects several streams
at once, returning a `Catalog`.

```cpp
using namespace pluss::kindle::literals;

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
score                    | Series       | Series[float64]  | 3       | [1, NA, 3]                                       
========================================================================================================================
```

Example 4 (edge case). Selecting an unknown key throws — with a different
message on the single-key overload than on the multi-name overloads.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
});
try {
  auto bad = ct.keys("nope");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << "single: " << msg.substr(0, msg.find('\n')) << "\n";
}
try {
  auto bad = ct.keys({"nope"});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << "multi: " << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
single: Catalog: key 'nope' not found
multi: Catalog::keys: unknown key 'nope'
```

## Notes

- See the important callout above — the single-key overload's return type is
  the most common source of confusion on this method.
- The no-argument overload returns a reference into the catalog's own
  storage — no copy.
- The multi-name overloads return a projected `Catalog`; pass a
  single-element list (`ct.keys({"score"})`) if you want a one-key `Catalog`
  instead of the single-key overload's bare `Stream`.
- Selecting an unknown key name throws, with a different message on the
  single-key overload (`"Catalog: key 'nope' not found"`) than the multi-name
  overloads (`"Catalog::keys: unknown key 'nope'"`) — see
  [`has()`](has.md) to check first.

## See Also

- [`Catalog::has`](has.md): check whether a key exists before selecting it.
- [`Catalog::drop`](drop.md): remove names instead of projecting them.
- [`Catalog::at`](at.md) / [`Catalog::get`](get.md): other single-stream
  extraction paths.
- [`Catalog::select`](select.md): static row-wise conditional choose between
  whole catalogs.

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
