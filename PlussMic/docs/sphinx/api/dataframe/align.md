# DataFrame::align

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::align(const AlignSpec& spec) -> DataFrame

DataFrame::align(const Thread& new_caselbl) -> DataFrame
```

Realign the current frame to a target case axis, a target feature axis, or both at once.

`align(...)` is the DataFrame-level reindexing surface for PlussMic. Use it when you already know the row labels, feature names, or both structures you want in the output and you want PlussMic to rebuild the frame to match that target.

This is especially useful in grading, reporting, and merge-preparation workflows where one table must be forced into the same visible shape as another before arithmetic, comparison, export, or presentation.

## Parameters

**spec**

Structured alignment description used by the full overload.

`AlignSpec` has three public fields:

- `caselbl`: optional target case labels. When present, the output rows are reordered and expanded to match these labels.
- `features`: optional target feature order. When non-empty, the output features are emitted in exactly this order.
- `fill`: optional numeric fill scalar. When provided, newly introduced numeric gaps are filled with that value instead of remaining null.

You do not need to populate every field. Leave the fields you do not care about at their defaults and specify only the axis you want to control.

**new_caselbl**

Replacement case axis used by the case-label-only overload.

Pass a note-backed or otherwise defined `Thread` containing the output row labels in the exact order you want. PlussMic will gather matching rows by label, insert missing rows for unmatched labels, and keep the existing feature set unchanged.

## Returns

`DataFrame`
    New frame whose case labels, feature order, and missing slots follow the requested alignment target.

    - Existing matching cells are carried over into their new positions.
    - Missing target rows or target features are introduced as missing values by default.
    - When `fill` is supplied, newly created numeric gaps are filled with that scalar instead of remaining null.

## See Also

- <a href="align_caselbl.html"><code>DataFrame::align_caselbl</code></a>: shorter case-axis-only surface when you are not also conforming features.
- <a href="conform.html"><code>DataFrame::conform</code></a>: align this frame to another frame's schema and labels in one call.
- <a href="../thread/align.html"><code>Thread::align</code></a>: lower-level alignment for a single extracted column.

## Notes

`align(const Thread& new_caselbl)` is the row-axis shortcut. It keeps the current feature set and only rebuilds the case axis.

`align(const AlignSpec& spec)` is the full structured form. Use it when you need to control target case labels, target feature order, and numeric fill behavior together.

If a requested feature does not exist in the source frame, PlussMic creates a new missing feature in the output. If that new feature is numeric and `fill` is supplied, the introduced values are filled with that scalar.

If a requested case label does not exist in the source frame, PlussMic inserts a new output row for that label. Matching rows are gathered by label rather than by original physical position.

This call is non-mutating. The source frame is left unchanged.

## Examples

Example 1. Realign rows to a new case-label order with the case-only overload.

```cpp
using namespace pluss::table;

auto base = DataFrame({
    {"A", pluss::make_series({1.0, 2.0, 3.0})},
    {"B", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"r1", "r2", "r3"}));

auto out = base.align(pluss::make_note({"r2", "missing", "r1"}));
```

### Output

```text
====================
        | A     B
--------------------
r2      | 2     20
missing | NA    NA
r1      | 1     10
--------------------
[3 cs x 2 feat]
```

Example 2. Fill newly introduced numeric rows instead of leaving them missing.

```cpp
using namespace pluss::table;

auto base = DataFrame({
    {"A", pluss::make_series({1.0, 2.0, 3.0})},
    {"B", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"r1", "r2", "r3"}));

auto out = base.align(DataFrame::AlignSpec{
    .caselbl = pluss::make_note({"r2", "missing", "r1"}),
    .fill = 0.0,
});
```

### Output

```text
====================
        | A     B
--------------------
r2      | 2     20
missing | 0     0
r1      | 1     10
--------------------
[3 cs x 2 feat]
```

Example 3. Reorder features and introduce one missing feature by name.

```cpp
using namespace pluss::table;

auto base = DataFrame({
    {"A", pluss::make_series({1.0, 2.0, 3.0})},
    {"B", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"r1", "r2", "r3"}));

auto out = base.align(DataFrame::AlignSpec{
    .features = {"B", "C", "A"},
});
```

### Output

```text
=======================
     | B     C     A
-----------------------
r1   | 10    NA    1
r2   | 20    NA    2
r3   | 30    NA    3
-----------------------
[3 cs x 3 feat]
```

Example 4. Conform both rows and features in one call.

```cpp
using namespace pluss::table;

auto base = DataFrame({
    {"A", pluss::make_series({1.0, 2.0, 3.0})},
    {"B", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"r1", "r2", "r3"}));

auto out = base.align(DataFrame::AlignSpec{
    .caselbl = pluss::make_note({"r3", "r1"}),
    .features = {"B", "C", "A"},
});
```

### Output

```text
=======================
     | B     C     A
-----------------------
r3   | 30    NA    3
r1   | 10    NA    1
-----------------------
[2 cs x 3 feat]
```

Example 5. Use `fill` together with feature conformance.

```cpp
using namespace pluss::table;

auto base = DataFrame({
    {"A", pluss::make_series({1.0, 2.0, 3.0})},
    {"B", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"r1", "r2", "r3"}));

auto out = base.align(DataFrame::AlignSpec{
    .caselbl = pluss::make_note({"r3", "r1"}),
    .features = {"B", "C", "A"},
    .fill = 0.0,
});
```

### Output

```text
=======================
     | B     C     A
-----------------------
r3   | 30    0     3
r1   | 10    0     1
-----------------------
[2 cs x 3 feat]
```

Example 6. Align one frame to another frame's case axis before downstream comparison or joining.

```cpp
using namespace pluss::table;

auto base = DataFrame({
    {"A", pluss::make_series({1.0, 2.0, 3.0})},
    {"B", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"r1", "r2", "r3"}));

auto case_driver = DataFrame({
    {"score", pluss::make_series({100.0, 200.0})},
}, pluss::make_note({"r3", "r1"}));

auto out = base.align(case_driver.caselbl());
```

### Output

```text
=================
     | A     B
-----------------
r3   | 3     30
r1   | 1     10
-----------------
[2 cs x 2 feat]
```

Example 7. Drop features you do not want by specifying a smaller target feature list.

```cpp
using namespace pluss::table;

auto base = DataFrame({
    {"A", pluss::make_series({1.0, 2.0, 3.0})},
    {"B", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"r1", "r2", "r3"}));

auto out = base.align(DataFrame::AlignSpec{
    .features = {"B"},
});
```

### Output

```text
===========
     | B
-----------
r1   | 10
r2   | 20
r3   | 30
-----------
[3 cs x 1 feat]
```

Example 8. Use `align(...)` as an explicit no-op when you want to normalize a pipeline step around known case labels.

```cpp
using namespace pluss::table;

auto base = DataFrame({
    {"A", pluss::make_series({1.0, 2.0, 3.0})},
    {"B", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"r1", "r2", "r3"}));

auto out = base.align(base.caselbl());
```

### Output

```text
=================
     | A     B
-----------------
r1   | 1     10
r2   | 2     20
r3   | 3     30
-----------------
[3 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="aggregate.html">
    <span class="label">Previous</span>
    <span class="title">aggregate</span>
  </a>
  <a class="next-link" href="align_caselbl.html">
    <span class="label">Next</span>
    <span class="title">align_caselbl</span>
  </a>
</div>
