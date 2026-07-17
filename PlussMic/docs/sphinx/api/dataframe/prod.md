# DataFrame::prod

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::prod(Axis axis = Axis::Case, bool skipnull = true) -> Thread
```

## What It Does

Computes per-feature or per-row products across numeric data.

## Parameters

**axis**
    Reduction axis. `Axis::Feature` computes one product per numeric feature, while `Axis::Case` computes one product per row across numeric features.

**skipnull**
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Compute products across rows instead of returning one product per numeric feature.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.prod(1, true);
```

### Output

```text
[2.0, 8.0, 15.0]
```

## Axis Rules

- `prod(render=false)` returns a compact summary view with one product per numeric feature.
- `prod(axis=0)` returns a numeric thread with one product per numeric feature.
- `prod(axis=1)` returns a numeric thread with one product per row across numeric features.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prep_sql.html">
    <span class="label">Previous</span>
    <span class="title">prep_sql</span>
  </a>
  <a class="next-link" href="ptypes.html">
    <span class="label">Next</span>
    <span class="title">ptypes</span>
  </a>
</div>
