# DataFrame::add_prefix

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::add_prefix(std::string_view prefix) -> DataFrame
```

Prepend the same text prefix to every feature name in the current DataFrame.

`DataFrame::add_prefix(...)` is a structural rename helper. It is useful when you want to stamp feature provenance, distinguish several projected views of the same source table, or prepare columns for a merge where name collisions would otherwise be ambiguous.

## Parameters

**prefix**

Text inserted at the start of every feature name.

- Use short namespace-like prefixes such as `exam_`, `left_`, or `clean_` when you want the resulting feature names to remain readable.
- Pass an empty string only when you intentionally want a no-op.

## Returns

`DataFrame`
    A new frame whose feature names all start with the requested prefix.

    Values, case labels, and row order are preserved.

## See Also

- <a href="add_suffix.html"><code>DataFrame::add_suffix</code></a>: append a shared suffix instead of a prefix.
- <a href="rename.html"><code>DataFrame::rename</code></a>: rename one feature at a time when you do not want a blanket prefix.
- <a href="#rename_features"><code>DataFrame::rename_features</code></a>: rename several explicit features with a mapping.

## Notes

This method rewrites only feature labels. It does not modify values, types, case labels, or title metadata.

If you want to prefix only a subset of columns, project that subset first, or rebuild the final frame from a prefixed view and the untouched columns you want to keep separate.

## Examples

Example 1. Add one prefix to every feature in the selected frame.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"units", pluss::make_series({10.0, 7.0, 12.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5})},
}).title("sales");

auto out = sales.add_prefix("exam_");
```

### Output

```text
=============================
     | exam_units  exam_price
-----------------------------
0    | 10          3
1    | 7           4
2    | 12          2.5
-----------------------------
[3 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
  <a class="next-link" href="add_suffix.html">
    <span class="label">Next</span>
    <span class="title">add_suffix</span>
  </a>
</div>
