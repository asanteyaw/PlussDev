# DataFrame::level

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::level(std::int64_t position) -> Thread

DataFrame::level(std::string_view name) -> Thread
```

## What It Does

Extracts one case-axis level as a full thread, which is useful after you build multi-level row labels with `set_cases({...})`.

## Parameters

**position**
    Zero-based case-level position to extract from a multi-level case axis. Negative positions count backward from the end.

**name**
    Case-level name to extract from a multi-level case axis. The returned thread has one value per row in the current frame order.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Extract one case level by its zero-based level position.

```cpp
auto grades = pluss::table::DataFrame({
    {"student", pluss::table::Thread(pluss::make_note({"Ada", "Ada", "Kojo", "Kojo"}))},
    {"term", pluss::table::Thread(pluss::make_note({"mid", "final", "mid", "final"}))},
    {"score", pluss::table::Thread(pluss::make_series({88.0, 91.0, 84.0, 89.0}))},
});
auto indexed = grades.set_cases({"student", "term"}, true, "|");

auto out = indexed.level(1);
```

### Output

```text
["mid", "final", "mid", "final"]
```
Example 2. Extract one case level by its level name.

```cpp
auto grades = pluss::table::DataFrame({
    {"student", pluss::table::Thread(pluss::make_note({"Ada", "Ada", "Kojo", "Kojo"}))},
    {"term", pluss::table::Thread(pluss::make_note({"mid", "final", "mid", "final"}))},
    {"score", pluss::table::Thread(pluss::make_series({88.0, 91.0, 84.0, 89.0}))},
});
auto indexed = grades.set_cases({"student", "term"}, true, "|");

auto out = indexed.level("student");
```

### Output

```text
["Ada", "Ada", "Kojo", "Kojo"]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lengthen.html">
    <span class="label">Previous</span>
    <span class="title">lengthen</span>
  </a>
  <a class="next-link" href="levels.html">
    <span class="label">Next</span>
    <span class="title">levels</span>
  </a>
</div>
