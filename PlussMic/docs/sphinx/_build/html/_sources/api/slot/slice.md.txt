# Slot::slice

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::slice(std::int64_t start, std::int64_t stop) -> Slot
```

## What It Does

Returns a contiguous positional slice `[start, stop)` from the slot.

## Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Inspect one contiguous positional slice from the current object.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.slice(0, 2);
```

### Output

```text
Slot([Series(1), Series(2), Series(3)])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
