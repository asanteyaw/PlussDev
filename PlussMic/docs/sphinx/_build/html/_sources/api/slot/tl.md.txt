# Slot::tl

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::tl() -> SlotTimelineAccessor
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`SlotTimelineAccessor`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.tl();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="to_json.html">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>
