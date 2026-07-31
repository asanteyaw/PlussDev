# SlotNoteAccessor::strip

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signatures

```cpp
SlotNoteAccessor::strip() -> Slot

SlotNoteAccessor::strip(std::string_view chars) -> Slot
```

## What It Does

Applies an elementwise cleanup or transformation while preserving the surrounding alignment, labels, and payload family whenever possible.

## Parameters

**chars**
    Characters to trim from the edge of each text value.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto notes = slot.nt();

auto out = notes.strip();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto notes = slot.nt();

auto out = notes.strip("units");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="starts.html">
    <span class="label">Previous</span>
    <span class="title">starts</span>
  </a>
  <a class="next-link" href="titlecase.html">
    <span class="label">Next</span>
    <span class="title">titlecase</span>
  </a>
</div>
