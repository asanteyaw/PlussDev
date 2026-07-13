# DataFrame::levels

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::levels() -> std::vector<std::string>
```

## What It Does

Lists the current case-level names in order from outermost to innermost.

## Parameters

`None`

## Returns

`std::vector<std::string>`
    Value produced by this call.

## Examples

Example 1. List the current case-level names in order.

```cpp
auto grades = pluss::table::DataFrame({
    {"student", pluss::table::Thread(pluss::make_note({"Ada", "Ada", "Kojo", "Kojo"}))},
    {"term", pluss::table::Thread(pluss::make_note({"mid", "final", "mid", "final"}))},
    {"score", pluss::table::Thread(pluss::make_series({88.0, 91.0, 84.0, 89.0}))},
});
auto indexed = grades.set_cases({"student", "term"}, true, "|");

auto out = indexed.levels();
```

### Output

```text
["student", "term"]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="level.html">
    <span class="label">Previous</span>
    <span class="title">level</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
