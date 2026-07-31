# Slot::defined

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::defined() -> bool
```

## What It Does

Reports whether the current object owns a concrete payload that can be used safely in later operations.

## Parameters

`None`

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Inspect the `defined` property of the current object.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.defined();
```

### Output

```text
true
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="concat.html">
    <span class="label">Previous</span>
    <span class="title">concat</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
