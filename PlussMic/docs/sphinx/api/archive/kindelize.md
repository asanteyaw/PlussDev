# Archive::kindelize

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::kindelize(const pluss::table::Atlas& atlas, const KindelizePolicy& policy = {}) -> Archive
```

## What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

## Parameters

**atlas**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**policy**
    Policy object that changes the structural behavior of this overload.

## Returns

`Archive`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = pluss::kindle::Archive::kindelize(atlas);
```

### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keys.html">
    <span class="label">Previous</span>
    <span class="title">keys</span>
  </a>
  <a class="next-link" href="label.html">
    <span class="label">Next</span>
    <span class="title">label</span>
  </a>
</div>
