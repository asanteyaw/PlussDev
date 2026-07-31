# Axel::empty

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::empty() -> bool
```

## What It Does

Reports whether the current object has no rows, entries, slots, or depth items.

## Parameters

`None`

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Inspect the `empty` property of the current object.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.empty();
```

### Output

```text
false
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="fieldaxis.html">
    <span class="label">Next</span>
    <span class="title">fieldaxis</span>
  </a>
</div>
