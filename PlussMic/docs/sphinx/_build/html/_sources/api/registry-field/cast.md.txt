# RegistryFieldAccessor::cast

[Back to RegistryFieldAccessor API Reference](../../registry_field_accessor_api_reference)

## Signatures

```cpp
RegistryFieldAccessor::cast() -> RegistryCastAccessor

RegistryFieldAccessor::cast(const pluss::table::ThreadCastOptions& options) -> RegistryCastAccessor
```

## What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`RegistryCastAccessor`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});
auto fields = rg.fields({"ids"});

auto out = fields.cast();
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
auto fields = rg.fields({"ids"});

auto out = fields.cast(df.features("load"));
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
