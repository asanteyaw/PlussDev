# SlotNoteAccessor::pad

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
SlotNoteAccessor::pad(std::int64_t width, pluss::table::TextAlign align = pluss::table::TextAlign::Left, char fill = ' ') -> Slot
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**width**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**align**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**fill**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto notes = slot.nt();

auto out = notes.pad(1, {}, {});
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize.html">
    <span class="label">Previous</span>
    <span class="title">normalize</span>
  </a>
  <a class="next-link" href="partition.html">
    <span class="label">Next</span>
    <span class="title">partition</span>
  </a>
</div>
