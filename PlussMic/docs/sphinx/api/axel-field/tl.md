# AxelFieldAccessor::tl

[Back to AxelFieldAccessor API Reference](../../axel_field_accessor_api_reference)

## Signature

```cpp
AxelFieldAccessor::tl() -> AxelTimelineAccessor
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`AxelTimelineAccessor`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto fields = axel.fields({"ids"});

auto out = fields.tl();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nt.html">
    <span class="label">Previous</span>
    <span class="title">nt</span>
  </a>
</div>
