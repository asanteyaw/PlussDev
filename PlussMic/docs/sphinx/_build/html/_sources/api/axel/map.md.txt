# Axel::map

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::map(const std::function<Registry(const Registry&)>& fn) -> Axel
```

## What It Does

Applies the same function to each registry and returns the transformed axel.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Apply a user function and inspect the transformed result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.map([](const pluss::bundle::Registry& registry) { return registry; });
```

### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="labels.html">
    <span class="label">Previous</span>
    <span class="title">labels</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
