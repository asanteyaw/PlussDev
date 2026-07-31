# AxelTimelineAccessor::time

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
AxelTimelineAccessor::time() -> Axel
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
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-01T09:10:00Z"), pluss::make_timeline("2024-01-01T09:40:00Z")})},
});
auto r1 = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-02T09:10:00Z"), pluss::make_timeline("2024-01-02T09:40:00Z")})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto timelines = axel.fields({"ts"}).tl();

auto out = timelines.time();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html">
    <span class="label">Previous</span>
    <span class="title">shift</span>
  </a>
  <a class="next-link" href="tz.html">
    <span class="label">Next</span>
    <span class="title">tz</span>
  </a>
</div>
