<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 11d8bc901e5b -->

# Catalog::apply_

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::apply_(std::string_view field, const std::function<Stream(const Stream&)>& func) -> Catalog&
```

## What It Does

The in-place counterpart to [`apply()`](apply.md) — mutates `*this` directly, replacing
`field`'s stream with `func`'s result, and returns a reference to it. [`apply()`](apply.md)
itself delegates to this method internally (after its own separate unknown-key check), so
the throw messages for an empty callback, an empty field name, or a length mismatch all
carry the `apply_` prefix even when triggered through `apply()`.

## Parameters

**field** (`std::string_view`)
    The stream name to transform.

**func** (`const std::function<Stream(const Stream&)>&`)
    Called once with the current stream at `field`; must return a defined `Stream` of the
    same length.

## Returns

`Catalog&`
    A reference to `*this`, after the update.

## Examples

Example 1. Transform a stream in place; the return value is a reference to the same object.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

ct.apply_("id", [](const pluss::kindle::Stream& s) {
  return pluss::kindle::Stream(s.as_series() * 10.0);
});
std::cout << ct;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [10, 20, 30]                                     
========================================================================================================================
```

## Notes

- Unlike [`apply()`](apply.md), `apply_()` does not pre-check `has(field)` itself — an
  unknown key surfaces as [`at()`](at.md)'s own `"Catalog: key '...' not found"` throw
  instead of `apply()`'s `"Catalog::apply: unknown key '...'"`.
- `func`'s returned stream must be defined and the same length as the input; throws
  `"Catalog::apply_: output stream length must match input stream length"` otherwise.

## See Also

- [`Catalog::apply`](apply.md): the non-mutating form, with an extra unknown-key check
  before delegating here.
- [`Catalog::apply_all_`](apply_all_.md): the whole-catalog, in-place counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="apply.html">
    <span class="label">Previous</span>
    <span class="title">apply</span>
  </a>
  <a class="next-link" href="apply_all.html">
    <span class="label">Next</span>
    <span class="title">apply_all</span>
  </a>
</div>
