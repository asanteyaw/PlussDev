# SlotNoteAccessor::findall

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signatures

```cpp
SlotNoteAccessor::findall(std::string_view pattern) -> pluss::table::DataFrame

SlotNoteAccessor::findall(std::string_view pattern, const pluss::table::TextOptions& options) -> pluss::table::DataFrame
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**pattern**
    Pattern string used for matching or extraction.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`pluss::table::DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto notes = slot.nt();

auto out = notes.findall("units");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass an explicit options object to control the operation.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto notes = slot.nt();

auto out = notes.findall("units", {});
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ends.html">
    <span class="label">Previous</span>
    <span class="title">ends</span>
  </a>
  <a class="next-link" href="fullmatch.html">
    <span class="label">Next</span>
    <span class="title">fullmatch</span>
  </a>
</div>
