# AxelNoteAccessor::normalize

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signature

```cpp
AxelNoteAccessor::normalize(pluss::table::UnicodeForm form = pluss::table::UnicodeForm::NFC) -> Axel
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**form**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")})},
});
auto r1 = pluss::bundle::Registry({
    {"city", pluss::bundle::Slot({pluss::make_note("Kumasi"), pluss::make_note("Takoradi")})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto notes = axel.fields({"city"}).nt();

auto out = notes.normalize({});
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matches.html">
    <span class="label">Previous</span>
    <span class="title">matches</span>
  </a>
  <a class="next-link" href="pad.html">
    <span class="label">Next</span>
    <span class="title">pad</span>
  </a>
</div>
