# Thread::compare_rows

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::compare_rows(std::int64_t lhs_row, std::int64_t rhs_row, bool nulls_last = true) -> int
```

Compare two row positions inside the same thread.

Use this when you need ordering or equality information between two entries without materializing a full boolean thread.

## Parameters

**lhs_row**

Left-hand row position.

**rhs_row**

Right-hand row position.

**nulls_last**

Null ordering policy used if one or both compared positions are missing.

## Returns

`int`
    Comparison code for the two requested positions.

## Notes

`0` means the two positions compare equal under the thread's comparison rules.

The sign of a nonzero result follows the thread's internal ordering semantics for the payload family.

## Examples

Example 1. Compare two positions inside one note-backed thread.

```cpp
using namespace pluss::table;

auto duplicated = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Tema"}));

auto out = duplicated.compare_rows(0, 2, true);
```

### Output

```text
0
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="compare.html">
    <span class="label">Previous</span>
    <span class="title">compare</span>
  </a>
  <a class="next-link" href="contains.html">
    <span class="label">Next</span>
    <span class="title">contains</span>
  </a>
</div>
