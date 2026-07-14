# DataFrame::defined

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::defined() -> bool
```

Report whether the frame currently owns a usable backing object.

This is the lightweight integrity check for `DataFrame` itself, not a missing-value scan across its cells.

Use it when you need to distinguish “this frame object exists and can be used” from “this frame contains missing values somewhere inside the data.”

## Parameters

`None`

## Returns

`bool`
    `true` when the frame object is defined and ready for ordinary table operations.

## Notes

`defined()` answers whether the frame exists as a real object. It does not mean every cell is non-missing.

Use missing-data methods such as `dropnull()`, `isnull()`, `notnull()`, or per-feature validity-aware logic when your question is about nulls rather than object existence.

`defined()` is therefore most useful around factory code, optional construction branches, or defensive API boundaries. It is not a substitute for data-quality checks.

## Examples

Example 1. Check whether a freshly constructed frame is defined.

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"score", pluss::make_series({88.0, 79.0, 91.0})},
});

auto out = grades.defined();
```

### Output

```text
true
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cumsum.html">
    <span class="label">Previous</span>
    <span class="title">cumsum</span>
  </a>
  <a class="next-link" href="describe.html">
    <span class="label">Next</span>
    <span class="title">describe</span>
  </a>
</div>
