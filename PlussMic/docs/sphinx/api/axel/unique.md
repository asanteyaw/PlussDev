# Axel::unique

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::unique() -> bool
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
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.unique();
```

### Output

```text
true
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
</div>
