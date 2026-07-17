# DataFrame::repeated

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::repeated(pluss::RangeView<std::string_view> features = {}) -> Thread
```

Build a boolean repeated-group mask.

`repeated(...)` marks every row whose key appears more than once. This differs from `duplicated(...)`, which marks duplicate occurrences according to its keep policy.

## Parameters

**features**

Feature subset that defines repeated groups.

Leave this empty to compare whole rows. Pass one or more features when repeated membership should be keyed only on those columns.

## Returns

`Thread`
    Boolean-like thread with one value per row. `true` means the row belongs to a group that appears more than once.

## See Also

- <a href="duplicated.html"><code>DataFrame::duplicated</code></a>: mark duplicate occurrences after the first, before the last, or all occurrences depending on keep policy.
- <a href="drop_duplicates.html"><code>DataFrame::drop_duplicates</code></a>: remove duplicate rows.

## Notes

For values `[A, B, A, C, B]`, `repeated()` returns `[true, true, true, false, true]`.

Use `duplicated(...)` when you need pandas-style duplicate follower semantics such as `[false, false, true, false, true]`.

## Examples

Example 1. Mark all rows belonging to repeated student groups.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Ada", "Chen", "Bola"})},
});

auto out = df.repeated({"student"});
```

### Output

```text
0  true
1  true
2  true
3  false
4  true
Name: repeated, dtype: Series
```

Example 2. Compare repeated groups against duplicate occurrences.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Ada", "Chen", "Bola"})},
});

auto followers = df.duplicated({"student"});
auto groups = df.repeated({"student"});
```

### Output

```text
followers: [false, false, true, false, true]
groups:    [true, true, true, false, true]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reorder.html">
    <span class="label">Previous</span>
    <span class="title">reorder</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>
