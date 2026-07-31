# Slot::size

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot::size() -> std::int64_t
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
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

auto out = slot.size();
```

### Output

```text
3
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
