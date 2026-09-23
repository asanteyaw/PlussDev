<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e1506df5b471 -->

# AxelNoteAccessor::encode

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
Axel AxelNoteAccessor::encode(std::string_view encoding = "base64") const
```

## What It Does

Encodes each bound field's text bytes using the named encoding, in
every registry frame, replacing each field's `Note` values with their
encoded text form. Applies
[`RegistryNoteAccessor::encode`](../registry-note/encode.md) to every
frame via [`Axel::map`](../axel/map.md).

## Parameters

**encoding** (`std::string_view`, defaults to `"base64"`)
    The name of the encoding scheme to apply. `"base64"` and `"hex"`
    are the two schemes exercised in the examples below.

## Returns

`Axel`
    A new axel with every bound field's text, in every frame, replaced
    by its encoded form; every unbound field unchanged.

## Examples

Example 1. Default call: base64-encode a text field.

```cpp
auto r0 = pluss::scribble::Registry({{"txt", pluss::scribble::Slot({pluss::make_note("hi")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"txt"}).nt().encode();
std::cout << std::get<pluss::Note>(out.registry(0).fields("txt").at(0)).at(0);
```

### Output

```text
aGk=
```

Example 2. Passing `"hex"` explicitly selects hexadecimal encoding
instead.

```cpp
auto r0 = pluss::scribble::Registry({{"txt", pluss::scribble::Slot({pluss::make_note("hi")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"txt"}).nt().encode("hex");
std::cout << std::get<pluss::Note>(out.registry(0).fields("txt").at(0)).at(0);
```

### Output

```text
6869
```

## Notes

- The default encoding is `"base64"` — pass `"hex"` (or any other
  supported scheme) explicitly to change it.
- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.
- Bound via [`Axel::nt()`](../axel/nt.md), which binds every field of
  every frame unless you project first with
  [`fields(keys)`](../axel/fields.md).

## See Also

- [`AxelNoteAccessor::normalize`](normalize.md): the other byte/text-level
  transform on this accessor.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

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
