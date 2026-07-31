# RegistryNoteAccessor::replace

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signatures

```cpp
RegistryNoteAccessor::replace(std::string_view from, std::string_view to) -> Registry

RegistryNoteAccessor::replace(std::string_view from, std::string_view to, const pluss::table::TextOptions& options) -> Registry
```

## What It Does

This is part of the text-processing surface for note-backed or text-oriented data.

## Parameters

**from**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

**to**
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`Registry`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});
auto notes = rg.fields({"city"}).nt();

auto out = notes.replace("units", "units");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass an explicit options object to control the operation.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});
auto notes = rg.fields({"city"}).nt();

auto out = notes.replace("units", "units", {});
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
  <a class="next-link" href="rstrip.html">
    <span class="label">Next</span>
    <span class="title">rstrip</span>
  </a>
</div>
