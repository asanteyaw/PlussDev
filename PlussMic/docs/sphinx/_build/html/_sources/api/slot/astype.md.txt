# Slot::astype

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::astype(pluss::ScalarType dtype) -> Slot
```

## What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

## Parameters

**dtype**
    Target scalar dtype used by this conversion overload.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.astype();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_kindle.html">
    <span class="label">Previous</span>
    <span class="title">as_kindle</span>
  </a>
  <a class="next-link" href="at.html">
    <span class="label">Next</span>
    <span class="title">at</span>
  </a>
</div>
