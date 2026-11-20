# Shelf::kindelize

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
pluss::kindle::Stream Shelf::kindelize()
```

## What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

## Parameters

`None`

## Returns

`pluss::kindle::Stream`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto shelf = pluss::table::Shelf(pluss::make_series({10.0, 20.0, 30.0}));

auto out = shelf.kindelize();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kind.html">
    <span class="label">Previous</span>
    <span class="title">kind</span>
  </a>
  <a class="next-link" href="kurtosis.html">
    <span class="label">Next</span>
    <span class="title">kurtosis</span>
  </a>
</div>
