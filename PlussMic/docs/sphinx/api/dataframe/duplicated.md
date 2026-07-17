# DataFrame::duplicated

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::duplicated(pluss::RangeView<std::string_view> features = {}, bool keep_first = true, bool keep_last = false) -> Thread
```

Build a boolean duplicate-occurrence mask instead of dropping rows immediately.

Use this when you want to inspect, count, or reuse duplicate occurrences before deciding what to do with those rows.

It is the duplicate-analysis route; `drop_duplicates(...)` is the duplicate-removal route.

## Parameters

**features**

Feature subset that defines duplicate groups.

Leave this empty to compare whole rows. Pass one or more features when duplicate membership should be keyed only on those columns.

**keep_first**

Whether the first row in each duplicate group should be treated as the surviving reference row.

With the default `true`, later matches are marked duplicate and the first occurrence is left unmarked.

**keep_last**

Whether the last row in each duplicate group should be treated as the surviving reference row instead.

When this is `true`, earlier matches are marked duplicate and the final occurrence is left unmarked.

## Returns

`Thread`
    Boolean-like thread with one value per row indicating duplicate occurrence status under the requested keep policy.

## See Also

- <a href="repeated.html"><code>DataFrame::repeated</code></a>: mark every row whose key appears more than once.
- <a href="drop_duplicates.html"><code>DataFrame::drop_duplicates</code></a>: drop the duplicate rows immediately once you know the policy you want.

## Notes

The returned thread aligns one-for-one with the original row axis, so you can inspect it directly or reuse it in downstream masking logic.

`duplicated(...)` does not change row order or remove data. It only reports duplicate occurrences.

`duplicated()` and `repeated()` intentionally differ. For `[A, B, A, C, B]`, `duplicated()` returns `[false, false, true, false, true]`, while `repeated()` returns `[true, true, true, false, true]`.

## Examples

Example 1. Mark duplicates while keeping the first occurrence as the survivor.

```cpp
using namespace pluss::table;

auto duplicates = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Ada", "Chen", "Bola"})},
    {"quiz", pluss::make_series({10.0, 9.0, 10.0, 7.0, 9.0})},
    {"final", pluss::make_series({80.0, 75.0, 80.0, 90.0, 76.0})},
});

auto out = duplicates.duplicated({"student", "quiz"});
```

### Output

```text
0  false
1  false
2  true
3  false
4  true
Name: duplicated, dtype: Series
```

Example 2. Mark duplicates while keeping the last occurrence instead.

```cpp
using namespace pluss::table;

auto duplicates = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Ada", "Chen", "Bola"})},
    {"quiz", pluss::make_series({10.0, 9.0, 10.0, 7.0, 9.0})},
    {"final", pluss::make_series({80.0, 75.0, 80.0, 90.0, 76.0})},
});

auto out = duplicates.duplicated({"student"}, false, true);
```

### Output

```text
0  true
1  true
2  false
3  false
4  false
Name: duplicated, dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dtypes.html">
    <span class="label">Previous</span>
    <span class="title">dtypes</span>
  </a>
  <a class="next-link" href="eq.html">
    <span class="label">Next</span>
    <span class="title">eq</span>
  </a>
</div>
