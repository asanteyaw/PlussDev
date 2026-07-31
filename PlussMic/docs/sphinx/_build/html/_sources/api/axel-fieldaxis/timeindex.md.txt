# AxelFieldAxisAccessor::timeindex

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
AxelFieldAxisAccessor::timeindex() -> Axel
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
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto fieldaxis = axel.fieldaxis();

auto out = fieldaxis.timeindex();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="suffix.html">
    <span class="label">Previous</span>
    <span class="title">suffix</span>
  </a>
  <a class="next-link" href="timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
