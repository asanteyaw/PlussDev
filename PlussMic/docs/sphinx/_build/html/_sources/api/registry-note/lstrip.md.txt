# RegistryNoteAccessor::lstrip

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signatures

```cpp
RegistryNoteAccessor::lstrip() -> Registry

RegistryNoteAccessor::lstrip(std::string_view chars) -> Registry
```

## What It Does

Applies an elementwise cleanup or transformation while preserving the surrounding alignment, labels, and payload family whenever possible.

## Parameters

**chars**
    Characters to trim from the edge of each text value.

## Returns

`Registry`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});
auto notes = rg.fields({"city"}).nt();

auto out = notes.lstrip();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});
auto notes = rg.fields({"city"}).nt();

auto out = notes.lstrip("units");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lower.html">
    <span class="label">Previous</span>
    <span class="title">lower</span>
  </a>
  <a class="next-link" href="matches.html">
    <span class="label">Next</span>
    <span class="title">matches</span>
  </a>
</div>
