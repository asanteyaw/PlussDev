# Axel::tail

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::tail(std::int64_t n) -> Axel
```

## What It Does

Returns the last `n` registries from the axel depth axis.

## Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

## Returns

`Axel`
    Value produced by this call.

## Examples

Example 1. Inspect the last part of the current object without changing the source data.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.tail(1);
```

### Output

```text
depth0 -> Registry(ids -> Slot([Series(1), Series(2)]))
depth1 -> Registry(ids -> Slot([Series(3), Series(4)]))
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="to_json.html">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>
