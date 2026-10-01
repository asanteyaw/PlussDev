# SlotCastAccessor::duration

[Back to SlotCastAccessor API Reference](../../slot_cast_accessor_api_reference)

## Signature

```cpp
SlotCastAccessor::duration() -> Slot
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
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto cast = slot.cast();

auto out = cast.duration();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="next-link" href="note.html">
    <span class="label">Next</span>
    <span class="title">note</span>
  </a>
</div>
