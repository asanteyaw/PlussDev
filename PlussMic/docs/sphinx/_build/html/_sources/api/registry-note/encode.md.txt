# RegistryNoteAccessor::encode

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
RegistryNoteAccessor::encode(std::string_view encoding = "base64") -> Registry
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**encoding**
    Text encoding used when reading or writing text-based files.

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

auto out = notes.encode("units");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="ends.html">
    <span class="label">Next</span>
    <span class="title">ends</span>
  </a>
</div>
