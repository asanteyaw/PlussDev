<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fc87e631b81d -->

# Catalog::filter

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::filter(const pluss::Series& condition) const
```

## What It Does

Keeps only the rows where a boolean mask `Series` is `true`, applied independently to every
stream in the catalog. [`where()`](where.md) is a pure alias for this same method.

```{important} The mask length must match each stream's length individually
`filter()` checks `condition`'s length against *each stream's own length*, not against
some catalog-wide row count. On a ragged catalog, a mask that matches one stream's length
but not another's throws — naming the specific key that didn't match. See Example 3.
```

## Parameters

**condition** (`const pluss::Series&`)
    A `Bool`-dtype mask, `true` for rows to keep. Must be defined, and must match the
    length of every stream it's applied to.

## Returns

`Catalog`
    A new catalog with every stream restricted to the rows where `condition` is `true`.

## Examples

Example 1. Keep every other row via an explicit boolean mask.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Ho", "Cape Coast"})),
});

auto mask = pluss::make_series({true, false, true, false, true}, pluss::codem::ScalarType::Bool);
auto out = ct.filter(mask);
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
id                       | Series       | Series[float64]  | 3       | [1, 3, 5]                                        
city                     | Note         | Note             | 3       | [Accra, Kumasi, Cape Coast]                      
========================================================================================================================
```

Example 2. Build the mask from one of the catalog's own streams, via
[`keys("id")`](keys.md).`as_series()` and a `Slate`-level comparison.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
});

auto mask = (ct.keys("id").as_series() > 2.0);
auto out = ct.filter(mask);
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
id                       | Series       | Series[float64]  | 3       | [3, 4, 5]                                        
========================================================================================================================
```

Example 3. A mask length mismatch throws, naming the offending stream.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Ho", "Cape Coast"})),
});

auto bad_mask = pluss::make_series({true, false}, pluss::codem::ScalarType::Bool);
try {
  auto out = ct.filter(bad_mask);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::filter: mask length must match stream length for key 'id'
```

## Notes

- [`where()`](where.md) is a pure alias — identical behavior, different name.
- The length check runs per-stream, not once for the whole catalog — see the important
  callout above.
- Requires a defined catalog and a defined `condition`; throws on either an undefined
  catalog or an undefined mask.

## See Also

- [`Catalog::where`](where.md): an alias for this exact method.
- [`Catalog::query`](query.md): build the boolean mask from a callback that receives the
  catalog itself, instead of constructing it beforehand.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="grep.html">
    <span class="label">Next</span>
    <span class="title">grep</span>
  </a>
</div>
