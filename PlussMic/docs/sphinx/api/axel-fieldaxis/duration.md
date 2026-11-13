# AxelFieldAxisAccessor::duration

[Back to AxelFieldAxisAccessor API Reference](../../axel_fieldaxis_accessor_api_reference)

## Signature

```cpp
Axel AxelFieldAxisAccessor::duration()
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
auto r0 = pluss::scribble::Registry({
    {"ids", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::scribble::Registry({
    {"ids", pluss::scribble::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::scribble::Axel({r0, r1});
auto fieldaxis = axel.fieldaxis();

auto out = fieldaxis.duration();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
