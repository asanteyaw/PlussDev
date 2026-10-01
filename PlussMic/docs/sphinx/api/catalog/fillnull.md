<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3b90e7494017 -->

# Catalog::fillnull

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::fillnull(double value) const -> Catalog
Catalog::fillnull(std::string_view value) const -> Catalog
Catalog::fillnull(const char* value) const -> Catalog
Catalog::fillnull(const pluss::Timeline& value) const -> Catalog
Catalog::fillnull(const pluss::Duration& value) const -> Catalog
```

## What It Does

Fills `NULL` cells across the catalog with `value` — but **only** in streams whose payload
kind matches `value`'s type. Every other stream passes through completely untouched, with
no error. This lets you call `ct.fillnull(0.0)` on a whole catalog and have it affect only
the numeric streams, in one call, without first selecting them by name.

```{important} Only streams matching value's kind are touched — silently, no throw
- `fillnull(double)` fills only `Series`-kind streams.
- `fillnull(std::string_view)` fills only `Note`-kind streams.
- `fillnull(const pluss::Timeline&)` fills only `Timeline`-kind streams.
- `fillnull(const pluss::Duration&)` fills only `Duration`-kind streams.

Streams of every other kind are returned exactly as they were — not skipped with a
warning, not thrown on, just left alone. This silent skip is specific to calling
`fillnull()` on the whole `Catalog`; the keyed accessor path
[`Catalog::keys(name)`](keys.md)`.fillnull(...)` follows the same rule now. A selected
stream whose kind does not match `value` is left untouched rather than throwing.
```

## Parameters

**value** (`double` / `std::string_view` / `const char*` / `const pluss::Timeline&` /
`const pluss::Duration&`)
    The fill value. Its C++ type selects both the overload and which streams are eligible
    to be filled — see the important callout above.

## Returns

`Catalog`
    A new catalog with `NULL` cells filled in every kind-matching stream; every other
    stream unchanged.

## Examples

Example 1. `fillnull(0.0)` fills only the numeric stream — the `Note`-kind stream's own
`NULL` is left as-is.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}),
        pluss::make_series(std::vector<bool>{true, true, false})),
});

auto out = ct.fillnull(0.0);
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
id                       | Series       | Series[float64]  | 3       | [1, 0, 3]                                        
city                     | Note         | Note             | 3       | [Accra, Tema, NA]                                
========================================================================================================================
```

Example 2. `fillnull("MISSING")` is the mirror image — it fills only the `Note`-kind
stream, leaving the numeric stream's `NULL` untouched.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}),
        pluss::make_series(std::vector<bool>{true, true, false})),
});

auto out = ct.fillnull("MISSING");
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
id                       | Series       | Series[float64]  | 3       | [1, NA, 3]                                       
city                     | Note         | Note             | 3       | [Accra, Tema, MISSING]                           
========================================================================================================================
```

## Notes

- Kind-selective and silent about it — see the important callout above. This is the
  single most important thing to know about this method before relying on it.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.
- To fill a specific named stream, use [`Catalog::keys(name)`](keys.md)`.fillnull(...)`
  instead; it preserves the rest of the catalog and uses the same kind-selective skip
  semantics on the selected stream(s).

## See Also

- [`Catalog::dropnull`](dropnull.md): remove rows with any `NULL`, instead of filling them.
- [`Catalog::ffill`](ffill.md) / [`Catalog::bfill`](bfill.md): fill using neighboring
  values instead of a fixed scalar.
- [`Catalog::keys`](keys.md): scope a fill (or any other transform) to an explicit
  selection of streams while preserving the rest of the catalog, using the same
  kind-selective skip semantics.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ffill.html">
    <span class="label">Previous</span>
    <span class="title">ffill</span>
  </a>
  <a class="next-link" href="filter.html">
    <span class="label">Next</span>
    <span class="title">filter</span>
  </a>
</div>
