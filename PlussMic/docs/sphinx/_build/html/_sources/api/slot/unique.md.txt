# Slot::unique

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::unique() -> bool
```

## What It Does

Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.

## Parameters

`None`

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Call the simplest overload.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.unique();
```

### Output

```text
true
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="type_counts.html">
    <span class="label">Previous</span>
    <span class="title">type_counts</span>
  </a>
  <a class="next-link" href="values.html">
    <span class="label">Next</span>
    <span class="title">values</span>
  </a>
</div>
