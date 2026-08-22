<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2518ab3c03fb -->

# SlotNoteAccessor::encode

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::encode(std::string_view encoding = "base64") const
```

## What It Does

Encodes each value's text bytes using the named encoding, replacing each
`Note` value with its encoded text form.

## Parameters

**encoding** (`std::string_view`, defaults to `"base64"`)
    The name of the encoding scheme. `"base64"` and `"hex"` are the two
    schemes exercised in the examples below.

## Returns

`Slot`
    A new slot, every value replaced by its encoded form.

## Examples

Example 1. Default call: base64-encode.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("hi")});
auto out = slot.nt().encode();
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
aGk=
```

Example 2. Passing `"hex"` explicitly.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("hi")});
auto out = slot.nt().encode("hex");
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
6869
```

## Notes

- The default encoding is `"base64"`.
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::normalize`](normalize.md): the other byte/text-level
  transform on this accessor.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
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
