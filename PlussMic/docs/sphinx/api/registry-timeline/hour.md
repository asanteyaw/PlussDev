# RegistryTimelineAccessor::hour

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
RegistryTimelineAccessor::hour() -> Registry
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`Registry`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto rg = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-01T09:10:00Z"), pluss::make_timeline("2024-01-02T09:40:00Z")})},
});
auto timelines = rg.fields({"ts"}).tl();

auto out = timelines.hour();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmt.html">
    <span class="label">Previous</span>
    <span class="title">fmt</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
