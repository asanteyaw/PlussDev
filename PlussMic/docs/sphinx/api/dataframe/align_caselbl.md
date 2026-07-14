# DataFrame::align_caselbl

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::align_caselbl(const Thread& new_caselbl) -> DataFrame

DataFrame::align_caselbl(const Thread& new_caselbl, std::optional<double> fill) -> DataFrame
```

Reorder the rows of a DataFrame so they follow a supplied case-label axis exactly.

`DataFrame::align_caselbl(...)` is the direct case-label reindexing helper for PlussMic. Use it when you already know the target row-label order and you want the frame returned in that order without writing a manual join or lookup loop.

## Parameters

**new_caselbl**

Replacement case-label axis in the exact row order you want.

- Pass a `Thread` of case labels.
- The output row order follows this thread from top to bottom.
- Repeated labels in `new_caselbl` repeat the matching source row in the output.

## Returns

`DataFrame`
    A new frame whose case axis matches `new_caselbl` exactly.

    The output row order follows the requested case-label sequence, and the returned frame carries `new_caselbl` as its case labels.

## See Also

- <a href="caselbl.html"><code>DataFrame::caselbl</code></a>: inspect the current case labels before building a target order.
- <a href="sort_cases.html"><code>DataFrame::sort_cases</code></a>: sort the existing case axis instead of supplying a custom target order.
- <a href="unify.html"><code>DataFrame::unify</code></a>: align frames by key features when you need join semantics rather than direct case-label reordering.

## Notes

Every requested case label must exist in the source frame unless you use the overload with an explicit fill value.

This method is best when the source case axis is unique. If the source contains duplicate case labels, the alignment picks the first matching row for each requested label.

`align_caselbl(...)` does not mutate the current frame. It returns a reordered copy.

## Examples

Example 1. Reorder rows to follow the case labels of another frame.

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"assignment", pluss::make_series({9.5, 8.0, 7.5})},
    {"final", pluss::make_series({28.0, 24.0, 21.5})},
}, pluss::make_note({"s1", "s2", "s3"})).title("grades");

auto scratch = DataFrame({
    {"score", pluss::make_series({88.0, 79.0, 91.0})},
}, pluss::make_note({"s2", "s3", "s1"})).title("scratch");

auto out = scratch.align_caselbl(grades.caselbl());
```

### Output

```text
============
     | score
------------
s1   | 91
s2   | 88
s3   | 79
------------
[3 cs x 1 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align.html">
    <span class="label">Previous</span>
    <span class="title">align</span>
  </a>
  <a class="next-link" href="all.html">
    <span class="label">Next</span>
    <span class="title">all</span>
  </a>
</div>
