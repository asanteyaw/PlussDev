# DataFrame::join

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::join(const DataFrame& other, std::string collision_suffix = "_right") -> DataFrame
```

## What It Does

Adds compatible features from another frame, aligning rows by the shared case axis.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

**collision_suffix**
    Suffix added when a second frame contributes a feature name that already exists on the left-hand side.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Attach columns from another case-aligned frame, adding the collision suffix only when names overlap.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.join(right);
```

### Output

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
