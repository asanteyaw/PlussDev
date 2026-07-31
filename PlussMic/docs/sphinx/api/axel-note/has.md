# AxelNoteAccessor::has

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
AxelNoteAccessor::has(std::string_view needle, const pluss::table::TextOptions& options = {}) -> Axel
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**needle**
    Substring or token to search for.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto r0 = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto r1 = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Kumasi"), pluss::make_note("Takoradi")})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto notes = axel.fields({"city"}).nt();

auto out = notes.has("units", {});
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fullmatch.html">
    <span class="label">Previous</span>
    <span class="title">fullmatch</span>
  </a>
  <a class="next-link" href="length.html">
    <span class="label">Next</span>
    <span class="title">length</span>
  </a>
</div>
