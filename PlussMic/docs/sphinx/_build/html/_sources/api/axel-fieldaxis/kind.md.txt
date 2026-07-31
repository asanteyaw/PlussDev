# AxelFieldAxisAccessor::kind

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
AxelFieldAxisAccessor::kind(SlotKind kind) -> Axel
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**kind**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Inspect the `kind` property of the current object.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});
auto fieldaxis = axel.fieldaxis();

auto out = fieldaxis.kind({});
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="like.html">
    <span class="label">Next</span>
    <span class="title">like</span>
  </a>
</div>
