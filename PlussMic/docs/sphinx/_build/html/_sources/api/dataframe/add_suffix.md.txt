# DataFrame::add_suffix

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::add_suffix(std::string_view suffix) -> DataFrame
```

Append the same text suffix to every feature name in the current DataFrame.

`DataFrame::add_suffix(...)` is the mirror of `add_prefix(...)`. Use it when the distinguishing context belongs at the end of the feature name, such as a semester stamp, scenario label, or derived-state marker.

## Parameters

**suffix**

Text appended to the end of every feature name.

- Common suffixes include labels such as `_winter`, `_clean`, `_pred`, or `_lag1`.
- Pass an empty string only when you intentionally want a no-op.

## Returns

`DataFrame`
    A new frame whose feature names all end with the requested suffix.

    Values, case labels, and row order are preserved.

## See Also

- <a href="add_prefix.html"><code>DataFrame::add_prefix</code></a>: prepend a shared label to every feature name.
- <a href="rename.html"><code>DataFrame::rename</code></a>: rename one feature at a time when you need an explicit target name.
- <a href="#rename_features"><code>DataFrame::rename_features</code></a>: rename several explicit features with a mapping.

## Notes

This method is purely structural. It changes the feature labels only.

Suffix stamping is especially useful after resampling, feature engineering, or side-by-side scenario comparisons where you want the original names to stay visible.

## Examples

Example 1. Add one suffix to every feature in the selected frame.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"units", pluss::make_series({10.0, 7.0, 12.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5})},
}).title("sales");

auto out = sales.add_suffix("_winter");
```

### Output

```text
=================================
     | units_winter  price_winter
---------------------------------
0    | 10            3
1    | 7             4
2    | 12            2.5
---------------------------------
[3 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add_prefix.html">
    <span class="label">Previous</span>
    <span class="title">add_prefix</span>
  </a>
  <a class="next-link" href="aggregate.html">
    <span class="label">Next</span>
    <span class="title">aggregate</span>
  </a>
</div>
