# SlotNoteAccessor::chars

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
SlotNoteAccessor::chars(std::int64_t start, std::optional<std::int64_t> stop = std::nullopt, std::int64_t step = 1) -> Slot
```

## What It Does

Extracts positional character slices from each note-backed text value in the current object.

## Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

**step**
    Positive frequency multiplier. For example, `6` with hourly frequency means six-hour bins.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto notes = slot.nt();

auto out = notes.chars(1, 1, 1);
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


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
