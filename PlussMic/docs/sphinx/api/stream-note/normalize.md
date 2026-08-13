<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e636e46d8316 -->

# StreamNoteAccessor::normalize

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::normalize(pluss::table::UnicodeForm form = pluss::table::UnicodeForm::NFC) const
```

## What It Does

Applies Unicode normalization to the bound stream's text, returning a new
stream. Normalization matters when text that looks identical can be
represented by different underlying byte sequences (e.g. a composed accented
character vs. the base letter plus a combining accent mark) — normalizing
first makes such text comparable and matchable consistently.

## Parameters

**form** (`pluss::table::UnicodeForm`, defaults to `NFC`)
    Which Unicode normalization form to apply — `NFC` (canonical composition,
    the default) or the library's other supported forms.

## Returns

`Stream`
    A new stream with its text normalized.

## Examples

Example 1. Plain ASCII text is unaffected by normalization (no multi-byte or
combining characters to normalize) — included as a baseline confirming the
call succeeds and returns unmodified `Note` output for such input.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra"}));
auto out = stream.nt().normalize();
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
0            | Accra                                                                                                    
========================================================================================================================
```

## Notes

- The default form, `NFC`, is the most common choice for general-purpose text
  comparison and storage.
- Has no visible effect on plain ASCII text, since ASCII has no
  composed/decomposed character distinction to normalize.

## See Also

- [`StreamNoteAccessor::encode`](encode.md): the other byte/text-level
  transform on this accessor.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::normalize`](../catalog-note/normalize.md): the
  whole-catalog equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matches.html">
    <span class="label">Previous</span>
    <span class="title">matches</span>
  </a>
  <a class="next-link" href="pad.html">
    <span class="label">Next</span>
    <span class="title">pad</span>
  </a>
</div>
