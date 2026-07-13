# Thread::contains

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::contains(std::string_view needle) -> pluss::Series
```

Return a boolean `Series` telling you whether each text entry contains a substring.

## Parameters

**needle**

Substring to search for inside each text value.

## Returns

`pluss::Series`
    Boolean series with one result per input row.

## Notes

This method is valid for note-backed text data.

## Examples

Example 1. Find which names contain a target substring.

```cpp
using namespace pluss::table;

auto names = Thread(pluss::make_note({"Ada", "BOLA", "cHeN"}));

auto out = names.contains("A");
```

### Output

```text
0  true
1  true
2  false
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="compare_rows.html">
    <span class="label">Previous</span>
    <span class="title">compare_rows</span>
  </a>
  <a class="next-link" href="corr.html">
    <span class="label">Next</span>
    <span class="title">corr</span>
  </a>
</div>
