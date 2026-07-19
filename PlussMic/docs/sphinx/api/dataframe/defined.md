<!-- pluss-hand-authored -->

# DataFrame::defined

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::defined() const noexcept -> bool
```

## What It Does

Reports whether the `DataFrame` handle currently owns a usable backing object at all
— not whether any cell inside it is missing. A default-constructed `DataFrame()` (no
columns argument) is the ordinary way to end up with an undefined frame; almost every
other constructor and factory method produces a defined one.

```{important} defined() is about object existence, not missing data
`defined()` answers "does this frame exist and can it be used?", not "does this frame
contain nulls?". For missing-data questions, use [`isnull()`](isnull.md),
[`notnull()`](notnull.md), or [`dropnull()`](dropnull.md) instead — a frame full of
nulls in every cell is still `defined() == true`.
```

## Parameters

`None`

## Returns

`bool`
    `true` if the frame owns a usable backing object and is safe to use in ordinary
    operations; `false` for a default-constructed, not-yet-initialized handle.

## Examples

Example 1. An ordinarily constructed frame is defined.

```cpp
using namespace pluss::table;

auto grades = DataFrame({
    {"student", pluss::make_note({"Ada", "Bola", "Chen"})},
    {"score",   pluss::make_series({88.0, 79.0, 91.0})},
});

auto out = grades.defined();
std::cout << std::boolalpha << out;
```

### Output

```text
true
```

Example 2. A default-constructed frame is not.

```cpp
auto empty_handle = DataFrame();
auto out_empty = empty_handle.defined();
std::cout << std::boolalpha << out_empty;
```

### Output

```text
false
```

`empty_handle` was constructed with no columns argument at all — it has no backing
object yet, unlike a frame constructed with an empty column list, which would be
defined but hold zero features.

## Notes

- `defined()` is `noexcept` and cheap — it's a pointer check, not a data scan.
- Most factory methods (`load_csv()`, arithmetic results, `copy()`, and so on) always
  return a defined frame; the main way to encounter an undefined one is the default
  constructor, or a moved-from handle.
- Calling most other methods on an undefined frame throws — `defined()` exists to let
  you check first, particularly around optional construction branches or defensive API
  boundaries.

## See Also

- [`DataFrame::isnull`](isnull.md) / [`DataFrame::notnull`](notnull.md): per-cell
  missing-data checks, unrelated to object existence.
- [`DataFrame::copy`](copy.md): always produces a defined frame, sharing storage with
  the source until either side is mutated.

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
