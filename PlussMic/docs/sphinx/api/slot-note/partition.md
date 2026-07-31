# SlotNoteAccessor::partition

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
SlotNoteAccessor::partition(std::string_view separator, std::string prefix = "part") -> pluss::table::DataFrame
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**separator**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**prefix**
    Text prefix inserted ahead of the existing label or value.

## Returns

`pluss::table::DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto notes = slot.nt();

auto out = notes.partition("units", {});
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pad.html">
    <span class="label">Previous</span>
    <span class="title">pad</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>
