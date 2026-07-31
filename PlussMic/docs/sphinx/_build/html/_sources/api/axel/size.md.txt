# Axel::size

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::size() -> std::int64_t
```

## What It Does

Returns a size/count summary for the current object without changing it.

## Parameters

`None`

## Returns

`std::int64_t`
    Integer count, size, or positional result.

## Examples

Example 1. Inspect the `size` property of the current object.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.size();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
