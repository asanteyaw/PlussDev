# Axel::labels

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::labels() -> const std::optional<Slot>&
```

## What It Does

Returns the current labels used to address fields, slots, rows, or depth items.

## Parameters

`None`

## Returns

`const std::optional<Slot>&`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.labels();
```

### Output

```text
["train", "test"]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kindelize.html">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
