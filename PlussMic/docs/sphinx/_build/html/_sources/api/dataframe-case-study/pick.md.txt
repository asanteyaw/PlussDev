<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eff6de7f78a9 -->

# DataFrameCaseStudyAccessor::pick

[Back to DataFrameCaseStudyAccessor API Reference](../../dataframe_case_study_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameCaseStudyAccessor::pick(const DataFrame::CasePick& pick) const
```

## What It Does

Selects rows from a named, possibly multi-level case axis. Unlike `labels()`, which
searches the rendered case-axis key, `pick()` can constrain one named level, several
named levels, or the complete ordered key. All supplied constraints are combined with
logical AND. At least one row must match.

## Parameters

**pick** (`const DataFrame::CasePick&`)
    The constraint to apply, using exactly one selector form (see CasePick Options
    below). An empty `CasePick` (no selector form populated) is an error.

## CasePick Options

| Field | Meaning | Default |
|---|---|---|
| `label` | One case-level name to match | empty |
| `value` | Value paired with `label` | empty |
| `names` | Several case-level names | empty |
| `values` | Values paired positionally with `names` | empty |
| `key` | Full key in the frame's case-level order | empty |
| `drop` | Remove matched level(s) from the result axis | `false` |

Use exactly one selector form: `label`/`value`, `names`/`values`, or `key`.
`names` and `values` must have equal lengths; `key` must contain exactly one value per
case level. Unknown levels, mixed selector forms, and a selector that matches no rows
are errors. Missing case-level values compare through the public `"NULL"` spelling.

## Returns

`DataFrame`
    A new frame containing every case that satisfies every supplied constraint. If
    `drop` is set, the matched level(s) are removed from the result's case axis;
    otherwise every original level is retained.

## Examples

Example 1. Match one named level; every row sharing that level's value is kept.

```cpp
using namespace pluss::table;
auto frame = DataFrame({
    {"region", Thread(pluss::make_note({"West", "West", "East"}))},
    {"account", Thread(pluss::make_note({"101", "102", "201"}))},
    {"sales", Thread(pluss::make_series({3, 5, 8}))},
}).label().set({"region", "account"});

auto west = frame.case_study().pick(
    DataFrame::CasePick{.label = "region", .value = "West"});
std::cout << west;
```

### Output

```text
================
         | sales
----------------
West101 | 3    
West102 | 5    
----------------
[2 cs x 1 feat]
```

Example 2. A full key, in the frame's case-level order, matches exactly one row.

```cpp
auto exact = frame.case_study().pick(
    DataFrame::CasePick{.key = {"East", "201"}});
std::cout << exact;
```

### Output

```text
================
         | sales
----------------
East201 | 8    
----------------
[1 cs x 1 feat]
```

Example 3. `drop = true` removes the matched level(s) from the result's case axis.

```cpp
auto stripped = frame.case_study().pick(DataFrame::CasePick{
    .names = {"region", "account"},
    .values = {"West", "101"},
    .drop = true,
});
std::cout << stripped;
```

### Output

```text
============
     | sales
------------
0    | 3    
------------
[1 cs x 1 feat]
```

Example 4. Several named constraints are paired positionally and combined with
logical AND.

```cpp
auto west_102 = frame.case_study().pick(DataFrame::CasePick{
    .names = {"region", "account"},
    .values = {"West", "102"},
});
std::cout << west_102;
```

### Output

```text
================
         | sales
----------------
West102 | 5    
----------------
[1 cs x 1 feat]
```

Example 5. `drop` removes only the level(s) used by the selector — the unused level
survives as the result's (now single-level) case axis.

```cpp
auto west_without_region = frame.case_study().pick(DataFrame::CasePick{
    .label = "region",
    .value = "West",
    .drop = true,
});
std::cout << west_without_region;
```

### Output

```text
============
     | sales
------------
101  | 3    
102  | 5    
------------
[2 cs x 1 feat]
```

Example 6. Each named level requires exactly one paired value; a length mismatch
fails before any row is examined.

```cpp
try {
  (void)frame.case_study().pick(DataFrame::CasePick{
      .names = {"region", "account"},
      .values = {"West"},
  });
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
DataFrame::case_study: names/values length mismatch
```

Example 7. A full key must cover every case level, in order.

```cpp
try {
  (void)frame.case_study().pick(
      DataFrame::CasePick{.key = {"West"}});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
DataFrame::case_study: full key length must match case-label count
```

## Notes

- `pick()` is the right terminal once case levels have been created with
  `label().set({...names...})`; use [`labels()`](labels.md) for a flat (single-string)
  case axis instead.
- Use exactly one of the three selector forms per call — mixing `label`/`value` with
  `names`/`values`, for example, is not a supported combination.
- A selector that matches no rows is an error, the same as an unknown key on
  [`labels()`](labels.md) — `pick()` never returns an unexpected zero-case frame.

## See Also

- [`DataFrameCaseStudyAccessor::labels`](labels.md): select by the rendered case-axis
  key as a single string, rather than by named level.
- [`DataFrame::label`](../dataframe/label.md): create the structured, multi-level case
  axis that `pick()` operates on, via `label().set({...names...})`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="labels.html"><span class="label">Previous</span><span class="title">labels</span></a>
  <a class="next-link" href="slice.html"><span class="label">Next</span><span class="title">slice</span></a>
</div>
