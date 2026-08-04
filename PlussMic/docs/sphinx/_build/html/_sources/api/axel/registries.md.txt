# Axel::registries

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::registries() -> const std::vector<Registry>&
```

## What It Does

Materializes the current members as a collection you can inspect or reuse elsewhere.

## Parameters

`None`

## Returns

`const std::vector<Registry>&`
    Borrowed collection of registries.

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

auto out = axel.registries();
```

### Output

```text
[Registry(depth0), Registry(depth1)]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nt.html">
    <span class="label">Previous</span>
    <span class="title">nt</span>
  </a>
  <a class="next-link" href="registry.html">
    <span class="label">Next</span>
    <span class="title">registry</span>
  </a>
</div>
