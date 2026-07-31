# Axel::depth

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::depth() -> std::int64_t
```

## What It Does

Returns a size/count summary for the current object without changing it.

## Parameters

`None`

## Returns

`std::int64_t`
    Integer count, size, or positional result.

## Examples

Example 1. Inspect the `depth` property of the current object.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.depth();
```

### Output

```text
2
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
