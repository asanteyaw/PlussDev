# DataFrame::drop_duplicates

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::drop_duplicates(pluss::RangeView<std::string_view> features = {}, bool keep_first = true, bool keep_last = false) -> DataFrame
```

Remove duplicate rows while controlling which occurrence should survive.

You can deduplicate on the whole row or on a selected feature subset.

This is the direct row-reduction route once you already know the duplicate policy you want.

## Parameters

**features**

Feature subset that defines duplicate groups.

- leave empty: compare whole rows,
- pass names: compare only those features while carrying all columns through in the output.

This matters whenever one subset defines row identity for your use case. For example, `{"student", "quiz"}` can define duplicate assessment submissions even if other columns differ.

**keep_first**

Whether the first row in each duplicate group should survive.

- `true`: keep the first occurrence,
- `false`: do not automatically preserve the first occurrence.

In the usual “keep first” route, the earliest matching row stays and later matches are removed.

**keep_last**

Whether the last row in each duplicate group should survive.

- `false`: use the first-occurrence route when `keep_first=true`,
- `true`: preserve the last occurrence instead.

Use this when later rows are considered the authoritative replacement for earlier ones.

## Returns

`DataFrame`
    Frame containing one surviving row for each duplicate group under the requested keep policy.

## See Also

- <a href="duplicated.html"><code>DataFrame::duplicated</code></a>: produce the duplicate mask first instead of dropping rows immediately.

## Notes

`keep_first` and `keep_last` let you express “keep first” versus “keep last” behavior directly.

The output row order follows the surviving rows in their original frame order. `drop_duplicates(...)` does not sort the result.

## Examples

Example 1. Drop duplicate rows while keeping the first observed occurrence.

```cpp
using namespace pluss::table;

auto duplicates = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Ada", "Chen", "Bola"})},
    {"quiz", pluss::make_series({10.0, 9.0, 10.0, 7.0, 9.0})},
    {"final", pluss::make_series({80.0, 75.0, 80.0, 90.0, 76.0})},
});

auto out = duplicates.drop_duplicates({"student", "quiz"});
```

### Output

```text
===========================
     | student  quiz  final
---------------------------
0    | Ada      10    80
1    | Bola     9     75
3    | Chen     7     90
---------------------------
[3 cs x 3 feat]
```

Example 2. Keep the last matching row instead.

```cpp
using namespace pluss::table;

auto duplicates = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Ada", "Chen", "Bola"})},
    {"quiz", pluss::make_series({10.0, 9.0, 10.0, 7.0, 9.0})},
    {"final", pluss::make_series({80.0, 75.0, 80.0, 90.0, 76.0})},
});

auto out = duplicates.drop_duplicates({"student"}, false, true);
```

### Output

```text
===========================
     | student  quiz  final
---------------------------
2    | Ada      10    80
3    | Chen     7     90
4    | Bola     9     76
---------------------------
[3 cs x 3 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop.html">
    <span class="label">Previous</span>
    <span class="title">drop</span>
  </a>
  <a class="next-link" href="drop_levels.html">
    <span class="label">Next</span>
    <span class="title">drop_levels</span>
  </a>
</div>
