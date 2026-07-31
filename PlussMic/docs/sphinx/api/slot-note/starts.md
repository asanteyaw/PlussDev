# SlotNoteAccessor::starts

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
SlotNoteAccessor::starts(std::string_view prefix) -> Slot
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**prefix**
    Text prefix inserted ahead of the existing label or value.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto notes = slot.nt();

auto out = notes.starts("units");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split.html">
    <span class="label">Previous</span>
    <span class="title">split</span>
  </a>
  <a class="next-link" href="strip.html">
    <span class="label">Next</span>
    <span class="title">strip</span>
  </a>
</div>
