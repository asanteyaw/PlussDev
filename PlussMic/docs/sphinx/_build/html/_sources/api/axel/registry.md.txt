# Axel::registry

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel::registry(std::int64_t i) -> Registry
```

## What It Does

Fetches one value, member, or slice from the current object without changing the source object.

## Parameters

**i**
    Zero-based positional index used by this overload.

## Returns

`Registry`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto r0 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});
auto r1 = pluss::bundle::Registry({
    {"ids", pluss::bundle::Slot({pluss::make_series(3), pluss::make_series(4)})},
});
auto axel = pluss::bundle::Axel({r0, r1});

auto out = axel.registry(0);
```

### Output

```text
ids  -> Slot([Series(1), Series(2), Series(3)])
city -> Slot([Note("Accra"), Note("Tema"), Note("Kumasi")])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="registries.html">
    <span class="label">Previous</span>
    <span class="title">registries</span>
  </a>
  <a class="next-link" href="rename.html">
    <span class="label">Next</span>
    <span class="title">rename</span>
  </a>
</div>
