# Thread::extract

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::extract(std::string_view pattern, pluss::RangeView<std::string_view> names = {}) -> DataFrame
```

Extract one or more regex capture groups from a note-backed thread into a DataFrame.

`extract(...)` is the structured regex parsing surface for text columns. Use it when one note column contains several embedded fields and you want each capture group broken out into its own feature.

## Parameters

**pattern**

Regular expression with one or more capture groups.

Only capture groups become output features. Surround the pieces you want to extract with parentheses.

**names**

Optional feature names for the capture groups.

- leave it empty to receive generated names such as `group_0`, `group_1`, ...
- pass one name per capture group when you want semantic output feature names

## Returns

`DataFrame`
    Frame with one note-backed feature per capture group.

    Rows that do not match the regex stay present in the output, but the unmatched capture-group cells are marked missing.

## See Also

- <a href="matches.html"><code>Thread::matches</code></a>: boolean regex test when you only need a mask.
- <a href="split.html"><code>Thread::split</code></a>: delimiter-based decomposition instead of regex capture groups.

## Notes

The pattern must contain at least one capture group. A regex with no capture groups throws.

If `names` is supplied, its length must match the number of capture groups exactly.

The output features are note-backed because regex capture groups produce text fragments.

## Examples

Example 1. Extract two capture groups with explicit output feature names.

```cpp
using namespace pluss::table;

auto parts = Thread(pluss::make_note({"A-100", "B-205", "C", "D-400"}));

auto out = parts.extract("([A-Z])-([0-9]+)", {"code", "id"});
```

### Output

```text
=================
     | code  id
-----------------
0    | A     100
1    | B     205
2    | NA    NA
3    | D     400
-----------------
[4 cs x 2 feat]
```

Example 2. Let PlussMic generate feature names for the capture groups.

```cpp
using namespace pluss::table;

auto parts = Thread(pluss::make_note({"A-100", "B-205", "C", "D-400"}));

auto out = parts.extract("([A-Z])-([0-9]+)");
```

### Output

```text
=======================
     | group_0  group_1
-----------------------
0    | A        100
1    | B        205
2    | NA       NA
3    | D        400
-----------------------
[4 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exp.html">
    <span class="label">Previous</span>
    <span class="title">exp</span>
  </a>
  <a class="next-link" href="ffill.html">
    <span class="label">Next</span>
    <span class="title">ffill</span>
  </a>
</div>
