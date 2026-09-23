<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d8ebb0c97ec9 -->

# StreamNoteAccessor::chars

[Back to StreamNoteAccessor API Reference](../../stream_note_accessor_api_reference)

## Signature

```cpp
Stream StreamNoteAccessor::chars(std::int64_t start, std::optional<std::int64_t> stop = std::nullopt, std::int64_t step = 1) const
```

## What It Does

Extracts a character-position substring from the bound stream's text, using
Python-style slicing semantics (`start`, `stop`, `step`), returning a new
stream.

## Parameters

**start** (`std::int64_t`)
    The starting character index (inclusive).

**stop** (`std::optional<std::int64_t>`, default `std::nullopt`)
    The ending character index (exclusive); `std::nullopt` means "to the end
    of the string."

**step** (`std::int64_t`, default `1`)
    The stride between extracted characters.

## Returns

`Stream`
    A new stream with each row's extracted character substring.

## Examples

Example 1. Extract the first three characters.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_note({"Accra"}));
auto out = stream.nt().chars(0, 3);
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
0            | Acc                                                                                                      
========================================================================================================================
```

## Notes

- `stop = std::nullopt` extracts to the end of the string.
- `step` allows extracting every Nth character within the slice.

## See Also

- [`StreamNoteAccessor::length`](length.md): the character count this
  method's indices are measured against.
- [`Stream::nt`](../stream/nt.md): the entry point this accessor is reached from.
- [`CatalogNoteAccessor::chars`](../catalog-note/chars.md): the whole-catalog
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="casefold.html">
    <span class="label">Previous</span>
    <span class="title">casefold</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
