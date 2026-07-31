# AxelTimelineAccessor::year

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
AxelTimelineAccessor::year() -> Axel
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-01T09:10:00Z"), pluss::make_timeline("2024-01-01T09:40:00Z")})},
});
auto r1 = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-02T09:10:00Z"), pluss::make_timeline("2024-01-02T09:40:00Z")})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto timelines = axel.fields({"ts"}).tl();

auto out = timelines.year();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekend.html">
    <span class="label">Previous</span>
    <span class="title">weekend</span>
  </a>
  <a class="next-link" href="yearday.html">
    <span class="label">Next</span>
    <span class="title">yearday</span>
  </a>
</div>
