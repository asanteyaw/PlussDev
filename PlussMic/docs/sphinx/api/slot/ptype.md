# Slot::ptype

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::ptype() -> std::string
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`std::string`
    Rendered string result.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.ptype();
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
  <a class="next-link" href="push.html">
    <span class="label">Next</span>
    <span class="title">push</span>
  </a>
</div>
