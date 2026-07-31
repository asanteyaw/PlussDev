# SlotNoteAccessor::count

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signatures

```cpp
SlotNoteAccessor::count(std::string_view pattern) -> Slot

SlotNoteAccessor::count(std::string_view pattern, const pluss::table::TextOptions& options) -> Slot
```

## What It Does

Returns a size/count summary for the current object without changing it.

## Parameters

**pattern**
    Pattern string used for matching or extraction.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Compute `count` and inspect the resulting summary value or summary object.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto notes = slot.nt();

auto out = notes.count("units");
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

auto out = notes.count("units", {});
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="chars.html">
    <span class="label">Previous</span>
    <span class="title">chars</span>
  </a>
  <a class="next-link" href="encode.html">
    <span class="label">Next</span>
    <span class="title">encode</span>
  </a>
</div>
