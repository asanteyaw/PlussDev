<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4c3e84934c25 -->

# Registry::from_kindle

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
static Registry Registry::from_kindle(const pluss::kindle::Catalog& catalog)
```

## What It Does

A `static` factory method — the inverse of [`to_kindle()`](to_kindle.md):
converts a `pluss::kindle::Catalog` into a `Registry`, one field per stream.
Since every `Catalog` stream is already uniform-kind, the result is always
stream-compatible.

## Parameters

**catalog** (`const pluss::kindle::Catalog&`)
    The catalog to convert. Must be defined.

## Returns

`Registry`
    A new registry with one field per stream in `catalog`.

## Examples

Example 1. Round-trip a registry through `to_kindle()` and back.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});

auto ct = rg.to_kindle();
auto rg2 = pluss::bundle::Registry::from_kindle(ct);

std::cout << "nfields=" << rg2.nfields() << " fields: ";
for (const auto& key : rg2.fields()) std::cout << key << " ";
```

### Output

```text
nfields=2 fields: id city
```

## Notes

- Throws `"Registry::from_kindle: catalog undefined"` if `catalog` isn't
  defined.
- Field order matches the source catalog's key order.
- Every field in the result is stream-compatible by construction, since it came
  from a uniform-kind `Catalog` stream.

## See Also

- [`Registry::to_kindle`](to_kindle.md): the inverse conversion.
- [`Slot::from_kindle`](../slot/from_kindle.md): the per-field conversion this
  method calls for each stream.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="get.html">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>
