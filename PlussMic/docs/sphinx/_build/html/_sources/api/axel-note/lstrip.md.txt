# AxelNoteAccessor::lstrip

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signatures

```cpp
AxelNoteAccessor::lstrip() -> Axel

AxelNoteAccessor::lstrip(std::string_view chars) -> Axel
```

## What It Does

Applies an elementwise cleanup or transformation while preserving the surrounding alignment, labels, and payload family whenever possible.

## Parameters

**chars**
    Characters to trim from the edge of each text value.

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto r0 = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto r1 = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Kumasi"), pluss::make_note("Takoradi")})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto notes = axel.fields({"city"}).nt();

auto out = notes.lstrip();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto r1 = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Kumasi"), pluss::make_note("Takoradi")})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto notes = axel.fields({"city"}).nt();

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
