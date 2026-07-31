# Slot::tail

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::tail(std::int64_t n) -> Slot
```

## What It Does

Returns the last `n` values from the slot.

## Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Inspect the last part of the current object without changing the source data.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.tail(2);
```

### Output

```text
Slot([Series(1), Series(2), Series(3)])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
