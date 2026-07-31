# AxelTimelineAccessor::floor

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
AxelTimelineAccessor::floor(std::string_view unit) -> Axel
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

**unit**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-01T09:10:00Z"), pluss::make_timeline("2024-01-01T09:40:00Z")})},
});
auto r1 = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-02T09:10:00Z"), pluss::make_timeline("2024-01-02T09:40:00Z")})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto timelines = axel.fields({"ts"}).tl();

auto out = timelines.floor("D");
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch.html">
    <span class="label">Previous</span>
    <span class="title">epoch</span>
  </a>
  <a class="next-link" href="fmt.html">
    <span class="label">Next</span>
    <span class="title">fmt</span>
  </a>
</div>
