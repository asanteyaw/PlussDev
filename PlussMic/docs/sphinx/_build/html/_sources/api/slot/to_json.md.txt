# Slot::to_json

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::to_json() -> std::string
```

## What It Does

Serializes or renders the current object into a text form you can inspect, log, or export.

## Parameters

`None`

## Returns

`std::string`
    Rendered string result.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.to_json();
```

### Output

```text
Slot([Series(1), Series(2), Series(3)])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tl.html">
    <span class="label">Previous</span>
    <span class="title">tl</span>
  </a>
  <a class="next-link" href="to_kindle.html">
    <span class="label">Next</span>
    <span class="title">to_kindle</span>
  </a>
</div>
