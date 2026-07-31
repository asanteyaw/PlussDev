# RegistryNoteAccessor::matches

[Back to RegistryNoteAccessor API Reference](../../registry_note_accessor_api_reference)

## Signature

```cpp
RegistryNoteAccessor::matches(std::string_view pattern, bool full = false) -> Registry
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**pattern**
    Pattern string used for matching or extraction.

**full**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

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

auto out = notes.matches("units", true);
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lstrip.html">
    <span class="label">Previous</span>
    <span class="title">lstrip</span>
  </a>
  <a class="next-link" href="normalize.html">
    <span class="label">Next</span>
    <span class="title">normalize</span>
  </a>
</div>
