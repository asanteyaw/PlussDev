# Slot::take

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::take(std::span<const std::int64_t> indices) -> Slot
```

## What It Does

Returns the values at the requested integer positions.

## Parameters

**indices**
    Integer positions used to select, reorder, or address entries.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Pass several integer positions together.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

std::vector<std::int64_t> picks{0, 2};
auto out = slot.take(picks);
```

### Output

```text
Slot([Series(1), Series(2), Series(3)])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="tl.html">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>
