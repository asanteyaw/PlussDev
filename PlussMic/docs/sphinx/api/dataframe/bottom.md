# DataFrame::bottom

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::bottom(std::string_view feature, std::int64_t n) -> DataFrame
```

Return the rows with the smallest values in one selected feature.

`DataFrame::bottom(...)` is the quick “lowest N rows” helper. It is useful for failure analysis, weakest-score inspection, or checking the lower tail of a distribution without manually sorting and slicing.

## Parameters

**feature**

Feature used as the ordering key.

- This is the column whose smallest values determine which rows survive.
- The feature should be compatible with ordering, typically numeric or temporal data.

**n**

Number of bottom rows to keep.

- Small values such as `1`, `3`, or `5` are useful for quick outlier or failure inspection.
- Larger values let you pull a broader low-end slice without manually sorting the full frame yourself.

## Returns

`DataFrame`
    A new frame containing the selected lowest rows.

    The output preserves the original row payloads and case labels for the chosen rows.

## See Also

- <a href="top.html"><code>DataFrame::top</code></a>: inspect the highest rows instead of the lowest ones.
- <a href="sort_values.html"><code>DataFrame::sort_values</code></a>: perform full explicit sorting when you need more control than `bottom(...)` exposes.
- <a href="head.html"><code>DataFrame::head</code></a>: preview the first rows of an already sorted frame.

## Notes

`bottom(feature, n)` sorts ascending by the selected feature, keeps nulls last, and then returns the first `n` rows.

Because nulls sort last, null rows are not selected unless there are not enough observed rows to satisfy `n`.

`n == 0` returns an empty frame with the same schema.

## Examples

Example 1. Return the two rows with the lowest final scores.

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen", "Dede"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5, 9.0})},
    {"final", pluss::make_series({28.0, 24.0, 21.5, 26.0})},
}, pluss::make_note({"s1", "s2", "s3", "s4"})).title("grades");

auto out = grades.bottom("final", 2);
```

### Output

```text
=================================
     | student  assignment  final
---------------------------------
s3   | Chen     7.5         21.5
s2   | Bola     8           24
---------------------------------
[2 cs x 3 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="bounded.html">
    <span class="label">Next</span>
    <span class="title">bounded</span>
  </a>
</div>
