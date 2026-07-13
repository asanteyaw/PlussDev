# DataFrame::drop_levels

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::drop_levels(pluss::RangeView<std::string_view> names) -> DataFrame

DataFrame::drop_levels(std::initializer_list<std::string_view> names) -> DataFrame
```

Remove one or more named levels from a structured case axis.

Use this after `set_cases(...)` when the frame already carries hierarchical case metadata and you want a simpler case key.

This is the “flatten the row index a little” operation for hierarchical cases.

## Parameters

**names**

Level names to remove from the current structured case axis.

Both overloads accept the same logical input; the difference is only whether you pass a reusable range or an inline initializer list.

## Returns

`DataFrame`
    Frame whose case metadata no longer includes the removed levels.

## See Also

- <a href="set_cases.html"><code>DataFrame::set_cases</code></a>: create structured case levels.
- <a href="reorder_levels.html"><code>DataFrame::reorder_levels</code></a>, <a href="rename_levels.html"><code>DataFrame::rename_levels</code></a>: manipulate structured case metadata without dropping it.

## Notes

`drop_levels(...)` changes case metadata, not feature columns. The data payload itself is preserved.

After the levels are removed, row grouping and row labels follow the remaining case levels only.

The initializer-list overload and the reusable-range overload perform the same operation. Choose based on whether you are passing names inline or reusing them across several calls.

## Examples

Example 1. Drop one named level from a structured case axis.

```cpp
using namespace pluss::table;

auto levels = DataFrame({
    {"campus", pluss::make_note({"north", "north", "south", "south"})},
    {"term", pluss::make_note({"fall", "spring", "fall", "spring"})},
    {"score", pluss::make_series({80.0, 82.0, 78.0, 88.0})},
}).set_cases({"campus", "term"}, true);

auto out = levels.drop_levels({"term"});
```

### Output

```text
=============
      | score
-------------
north | 80
north | 82
south | 78
south | 88
-------------
[4 cs x 1 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop_duplicates.html">
    <span class="label">Previous</span>
    <span class="title">drop_duplicates</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
