# Slot::values

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::values() -> const std::vector<Value>&
```

## What It Does

Materializes the current members as a collection you can inspect or reuse elsewhere.

## Parameters

`None`

## Returns

`const std::vector<Value>&`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.values();
```

### Output

```text
Slot([Series(1), Series(2), Series(3)])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unique.html">
    <span class="label">Previous</span>
    <span class="title">unique</span>
  </a>
</div>
