# AxelNoteAccessor::titlecase

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
AxelNoteAccessor::titlecase() -> Axel
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

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

auto out = notes.titlecase();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip.html">
    <span class="label">Previous</span>
    <span class="title">strip</span>
  </a>
  <a class="next-link" href="upper.html">
    <span class="label">Next</span>
    <span class="title">upper</span>
  </a>
</div>
