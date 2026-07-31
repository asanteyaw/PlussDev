# Slot::max

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::max() -> Slot
```

## What It Does

Use this when you want a summary view instead of carrying the full input forward unchanged.

## Parameters

`None`

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Compute `max` and inspect the resulting summary value or summary object.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.max();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kindelize.html">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="mean.html">
    <span class="label">Next</span>
    <span class="title">mean</span>
  </a>
</div>
