# SlotCastAccessor::timeindex

[Back to SlotCastAccessor API Reference](../../slot_cast_accessor_api_reference)

## Signature

```cpp
SlotCastAccessor::timeindex() -> Slot
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto cast = slot.cast();

auto out = cast.timeindex();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
  <a class="next-link" href="timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
