<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cc955795f7de -->

# CatalogNoteAccessor::encode

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::encode(std::string_view encoding = "base64") const
```

## What It Does

Encodes every bound key's text bytes using the named encoding, returning a new
catalog with those keys' `Note` values replaced by their encoded text form.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**encoding** (`std::string_view`, defaults to `"base64"`)
    The name of the encoding scheme to apply. `"base64"` and `"hex"` are the two
    schemes exercised in the examples below.

## Returns

`Catalog`
    A new catalog with every bound key's text replaced by its encoded form.

## Examples

Example 1. Default call: base64-encode a text key.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "secret"_k = pluss::kindle::Stream(pluss::make_note({"hi"})),
});
auto out = ct.nt().encode();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [secret]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
secret                   | Note         | Note             | 1       | [aGk=]                                           
========================================================================================================================
```

## Notes

- See the important callout above.
- The default encoding is `"base64"` — pass `"hex"` (or any other supported scheme)
  explicitly to change it.

## See Also

- [`CatalogNoteAccessor::normalize`](normalize.md): the other byte/text-level
  transform on this accessor.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::encode`](../registry-note/encode.md): the bundle-model
  equivalent, which also verifies the `"hex"` scheme's exact output.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="ends.html">
    <span class="label">Next</span>
    <span class="title">ends</span>
  </a>
</div>
