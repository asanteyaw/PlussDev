# DataFrame::astype

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::astype(pluss::RangeView<std::pair<std::string_view, pluss::ScalarType>> feature_dtypes) -> DataFrame

DataFrame::astype(pluss::RangeView<std::string_view> features, pluss::ScalarType dtype) -> DataFrame

DataFrame::astype(std::initializer_list<std::pair<std::string_view, pluss::ScalarType>> feature_dtypes) -> DataFrame
```

Cast one or more existing features to new scalar dtypes.

`DataFrame::astype(...)` is the typed conversion surface for DataFrame features. Use it when you want to normalize imported columns, switch numeric precision, or convert flag-like numeric columns into booleans before later filtering or aggregation.

## Parameters

**features**

Feature names to cast when you want one shared target dtype.

- Every listed feature is cast to the same `dtype`.
- Features not listed are left unchanged.

**dtype**

Shared target scalar dtype for the feature-list overload.

Common choices include:

- `pluss::ScalarType::Bool`
- `pluss::ScalarType::Int64`
- `pluss::ScalarType::Float64`

Pick the narrowest dtype that still matches the downstream operations you plan to run.

**feature_dtypes**

Collection of `(feature_name, dtype)` pairs for mixed casting.

- Use this overload when each selected feature should land on a different dtype.
- Each pair targets one existing feature explicitly.

## Returns

`DataFrame`
    A new frame with the requested feature casts applied.

    Features not mentioned in the cast request are preserved unchanged.

## See Also

- <a href="#cast_series"><code>DataFrame::cast_series</code></a>: force selected features onto the Series payload family before working numerically.
- <a href="#cast_note"><code>DataFrame::cast_note</code></a>: force selected features onto the Note payload family before text-oriented operations.
- <a href="../thread/astype.html"><code>Thread::astype</code></a>: cast one thread directly outside a frame.

## Notes

Use the feature-list overload when several features should land on the same target dtype.

Use the mapping overload when each feature needs its own target dtype.

This method returns a new frame. It does not mutate the current DataFrame in place.

## Examples

Example 1. Cast one selected feature to a shared target dtype.

```cpp
using namespace pluss::table;

auto raw = DataFrame({
    {"student_id", pluss::make_series({101.0, 102.0, 103.0})},
    {"passed", pluss::make_series({1.0, 0.0, 1.0})},
}).title("raw");

auto out = raw.astype({"passed"}, pluss::ScalarType::Bool);
```

### Output

```text
=========================
     | student_id  passed
-------------------------
0    | 101         true
1    | 102         false
2    | 103         true
-------------------------
[3 cs x 2 feat]
```

Example 2. Cast several features to different dtypes in one call.

```cpp
using namespace pluss::table;

auto raw = DataFrame({
    {"student_id", pluss::make_series({101.0, 102.0, 103.0})},
    {"passed", pluss::make_series({1.0, 0.0, 1.0})},
}).title("raw");

auto out = raw.astype({
    {"student_id", pluss::ScalarType::Int64},
    {"passed", pluss::ScalarType::Bool},
});
```

### Output

```text
=========================
     | student_id  passed
-------------------------
0    | 101         true
1    | 102         false
2    | 103         true
-------------------------
[3 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="assign.html">
    <span class="label">Previous</span>
    <span class="title">assign</span>
  </a>
  <a class="next-link" href="bfill.html">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>
