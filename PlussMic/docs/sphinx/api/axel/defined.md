# Axel::defined

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::defined() -> bool
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.defined();
```

### Output

```text
true
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="concat_depth.html">
    <span class="label">Previous</span>
    <span class="title">concat_depth</span>
  </a>
  <a class="next-link" href="depth.html">
    <span class="label">Next</span>
    <span class="title">depth</span>
  </a>
</div>
