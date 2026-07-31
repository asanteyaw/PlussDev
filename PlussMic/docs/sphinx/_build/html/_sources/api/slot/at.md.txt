# Slot::at

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::at(std::int64_t i) -> const Value&
```

## What It Does

Fetches one value, member, or slice from the current object without changing the source object.

## Parameters

**i**
    Zero-based positional index used by this overload.

## Returns

`const Value&`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.at(0);
```

### Output

```text
Series(1)
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
