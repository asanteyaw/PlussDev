<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b4626120b46d -->

# StreamNoteAccessor::encode

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::encode(std::string_view encoding = "base64") const
```

## What It Does

Encodes the bound stream's text bytes using the named encoding, returning a
new stream with the encoded text.

## Parameters

**encoding** (`std::string_view`, defaults to `"base64"`)
    The name of the encoding scheme to apply. `"base64"` and `"hex"` are the
    two schemes exercised in the examples below.

## Returns

`Stream`
    A new stream with its text replaced by its encoded form.

## Examples

Example 1. Default call: base64-encode a text stream.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"hi"}));
auto out = stream.nt().encode();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=1
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | aGk=                                                                                                     
========================================================================================================================
```

## Notes

- The default encoding is `"base64"` — pass `"hex"` (or any other supported
  scheme) explicitly to change it.

## See Also

- [`StreamNoteAccessor::normalize`](normalize.md): the other byte/text-level
  transform on this accessor.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::encode`](../catalog-note/encode.md): the
  whole-catalog equivalent.

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
