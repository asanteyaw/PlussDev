<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2d6e6d1aa081 -->

# DataFrameCaseStudyAccessor::where

[Back to DataFrameCaseStudyAccessor API Reference](../../dataframe_case_study_accessor_api_reference)

## Signatures

```cpp
DataFrame DataFrameCaseStudyAccessor::where(const Slate& values) const
DataFrame DataFrameCaseStudyAccessor::where(const pluss::Series& values) const
```

## What It Does

Keeps the cases selected by a one-dimensional Boolean mask. A `true` lane retains
the corresponding row and a `false` lane removes it. The surviving rows remain in
source order, all frame metadata and structured case labels are filtered with them,
and every feature is retained until an optional chained `features(...)` projection.

The two overloads differ in alignment. A bare `Series` is strictly positional: it must
be 1D, `Bool`-dtype, and contain exactly one value per source case, or the call
throws. A `Slate` is a table-aware mask: its case labels are aligned to the frame's
case axis before filtering, and invalid mask lanes are treated as `false`. Prefer the
`Slate` form when the predicate came from a frame feature or another labeled table
operation; prefer the `Series` form when an exact positional match is the intended
contract and a length mismatch should be a hard error rather than silently aligned.

`where()` is the case-selection spelling within this accessor. Root `filter()` is the
more direct choice when no other case-study terminal is involved; both ultimately use
the same filtering kernel.

## Parameters

**values** (`const Slate&` / `const pluss::Series&`)
    The Boolean mask. The `Slate` overload must be `Series`-kind with `Bool` dtype
    and is aligned to the frame's case axis before filtering (a lane with no aligned
    source value is treated as `false`). The `Series` overload must be 1D, `Bool`
    dtype, and exactly `ncases()` long, or the call throws.

## Returns

`DataFrame`
    A new frame containing every case whose mask lane is `true`, in source order. A
    mask with no `true` lanes returns a valid, zero-case frame with the original
    feature schema.

## Examples

Example 1. A `Slate` mask built from a frame feature's own comparison — the common
case, since the mask and the frame already share a case axis.

```cpp
using namespace pluss::table;
auto frame = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"}))},
    {"units", Slate(pluss::make_series({10, 7, 12, 5}))},
});

auto busy = frame.case_study().where(frame.features("units") >= 10)
                 .features({"city", "units"});
std::cout << busy;
```

### Output

```text
====================
     | city    units
--------------------
0    | Accra   10   
2    | Kumasi  12   
--------------------
[2 cs x 2 feat]
```

Example 2. A standalone Boolean `Series` is interpreted strictly positionally.

```cpp
auto positional = pluss::make_series({false, true, true, false});
auto middle = frame.case_study().where(positional);
std::cout << middle;
```

### Output

```text
====================
     | city    units
--------------------
1    | Tema    7    
2    | Kumasi  12   
--------------------
[2 cs x 2 feat]
```

Example 3. No selected lanes is a valid result with zero cases and the original
schema.

```cpp
auto none = frame.case_study().where(
    pluss::make_series({false, false, false, false}));
std::cout << "ncases=" << none.ncases() << "\n";
```

### Output

```text
ncases=0
```

Example 4. Mask dtype is strict — an `Int64` mask fails even though its values are
`0`/`1`, because implicit numeric truth conversion would make row removal difficult
to audit.

```cpp
try {
  (void)frame.case_study().where(pluss::make_series({1, 0, 1, 0}));
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
DataFrame::filter: mask dtype must be Bool
```

Example 5. The `Series` overload's length must match `ncases()` exactly — unlike the
`Slate` overload's case-label alignment, a shorter or longer mask is a hard error,
never silently truncated or padded.

```cpp
try {
  (void)frame.case_study().where(pluss::make_series({true, false}));
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
DataFrame::filter: mask length mismatch
```

## Notes

- The `Series` overload is strictly positional: 1D, `Bool` dtype, exactly `ncases()`
  elements, or it throws. It never aligns by case label and never silently resizes.
- The `Slate` overload aligns by case label instead of requiring an exact length
  match; a mask lane with no corresponding source case is simply treated as `false`.
  Prefer it when the mask came from the frame itself (a comparison, another table
  operation) rather than a hand-built buffer.
- `where()` and root [`filter()`](../dataframe/filter.md) use the same filtering
  kernel — prefer `where()` when already chaining through `case_study()`, and
  `filter()` directly otherwise.

## See Also

- [`DataFrame::filter`](../dataframe/filter.md): the root-level equivalent, reached
  without going through this accessor.
- [`DataFrameCaseStudyAccessor::at`](at.md) / [`take`](take.md): position-based
  selection, for when the decision isn't already a Boolean mask.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html"><span class="label">Previous</span><span class="title">take</span></a>
</div>
