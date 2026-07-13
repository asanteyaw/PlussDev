# DataFrame::update

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::update(const DataFrame& other, bool overwrite = true) -> DataFrame
```

## What It Does

Overlays matching features from another frame onto this frame using aligned cases.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

**overwrite**
    When `true`, matching values from the right-hand side overwrite existing left-hand values during update-style operations.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Overlay another aligned frame onto the current one and resolve collisions with the overwrite switch.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.update(right);
```

### Output

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unstack.html">
    <span class="label">Previous</span>
    <span class="title">unstack</span>
  </a>
  <a class="next-link" href="var.html">
    <span class="label">Next</span>
    <span class="title">var</span>
  </a>
</div>
