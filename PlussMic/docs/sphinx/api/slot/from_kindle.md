# Slot::from_kindle

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::from_kindle(const pluss::kindle::Stream& stream) -> Slot
```

## What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

## Parameters

**stream**
    Stream payload supplied to this overload.

## Returns

`Slot`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = pluss::bundle::Slot::from_kindle(stream);
```

### Output

```text
Slot([Series(1), Series(2), Series(3)])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
