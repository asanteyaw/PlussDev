# SlotNoteAccessor::lower

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
SlotNoteAccessor::lower() -> Slot
```

## What It Does

Applies an elementwise cleanup or transformation while preserving the surrounding alignment, labels, and payload family whenever possible.

## Parameters

`None`

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto notes = slot.nt();

auto out = notes.lower();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="length.html">
    <span class="label">Previous</span>
    <span class="title">length</span>
  </a>
  <a class="next-link" href="lstrip.html">
    <span class="label">Next</span>
    <span class="title">lstrip</span>
  </a>
</div>
