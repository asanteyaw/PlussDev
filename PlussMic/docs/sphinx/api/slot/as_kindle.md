# Slot::as_kindle

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::as_kindle() -> pluss::kindle::Stream
```

## What It Does

Converts or reinterprets the current payload as another public Pluss type so downstream operations route through the right text, numeric, or temporal surface.

## Parameters

`None`

## Returns

`pluss::kindle::Stream`
    Value produced by this call.

## Examples

Example 1. Convert the current object into the `kindle` representation before continuing with another type-specific workflow.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.as_kindle();
```

### Output

```text
Series([10.0, 20.0, 30.0])
```


<div class="pluss-inline-prev-next">
  <a class="next-link" href="astype.html">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>
