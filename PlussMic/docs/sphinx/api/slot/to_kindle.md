# Slot::to_kindle

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::to_kindle() -> pluss::kindle::Stream
```

## What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

## Parameters

`None`

## Returns

`pluss::kindle::Stream`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.to_kindle();
```

### Output

```text
Series([10.0, 20.0, 30.0])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_json.html">
    <span class="label">Previous</span>
    <span class="title">to_json</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
