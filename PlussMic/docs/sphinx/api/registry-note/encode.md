<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 438368a702b2 -->

# RegistryNoteAccessor::encode

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
Registry RegistryNoteAccessor::encode(std::string_view encoding = "base64") const
```

## What It Does

Encodes each bound field's text bytes using the named encoding, replacing
each field's `Note` values with their encoded text form.

## Parameters

**encoding** (`std::string_view`, defaults to `"base64"`)
    The name of the encoding scheme to apply. `"base64"` and `"hex"` are the
    two schemes exercised in the examples below.

## Returns

`Registry`
    A new registry with every bound field's text replaced by its encoded
    form; every unbound field unchanged.

## Examples

Example 1. Default call: base64-encode a text field.

```cpp
auto rg = pluss::bundle::Registry({
    {"txt", pluss::bundle::Slot({pluss::make_note("hi")})},
});

auto out = rg.fields({"txt"}).nt().encode();
std::cout << std::get<pluss::Note>(out.fields("txt").at(0)).at(0);
```

### Output

```text
aGk=
```

Example 2. Passing `"hex"` explicitly selects hexadecimal encoding instead.

```cpp
auto rg = pluss::bundle::Registry({
    {"txt", pluss::bundle::Slot({pluss::make_note("hi")})},
});

auto out = rg.fields({"txt"}).nt().encode("hex");
std::cout << std::get<pluss::Note>(out.fields("txt").at(0)).at(0);
```

### Output

```text
6869
```

## Notes

- The default encoding is `"base64"` — pass `"hex"` (or any other supported
  scheme) explicitly to change it.
- Requires every bound field to be `Note`-kind — a non-`Note` field throws
  `"Slot::nt: slot kind must be Note"`.
- Bound via [`Registry::nt()`](../registry/nt.md), which binds every field of
  the registry unless you project first with
  [`fields(keys)`](../registry/fields.md).

## See Also

- [`RegistryNoteAccessor::normalize`](normalize.md): the other byte/text-level
  transform on this accessor.
- [`Registry::nt`](../registry/nt.md): the entry point this accessor is
  reached from.

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
