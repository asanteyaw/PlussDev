<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f5d6d8112642 -->

# DataFrameCaseStudyAccessor::at

[Back to DataFrameCaseStudyAccessor API Reference](../../dataframe_case_study_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameCaseStudyAccessor::at(std::int64_t row) const
```

## What It Does

Selects exactly one case by position while preserving the result as a one-row
`DataFrame`. Nonnegative positions count from the front; negative positions count
from the end, so `-1` is the last case. The selected case label, all feature dtypes,
the frame title, and feature order survive. Unlike `slice()`, `at()` does not clamp an
out-of-range position: it fails before constructing a result.

Use `at()` when one physical row is the contract. Use `labels()` when a case-axis value
is the contract, because one label may match several rows.

## Parameters

**row** (`std::int64_t`)
    The case position to select. `0` is the first case; `-1` is the last. Must satisfy
    `-ncases() <= row < ncases()` after normalization, or the call throws.

## Returns

`DataFrame`
    A new, one-case frame with every feature, dtype, and the selected case's label
    preserved. The source frame is unchanged.

## Examples

Example 1. Select the first case by a nonnegative position.

```cpp
using namespace pluss::table;
auto frame = DataFrame({
    {"city", Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"units", Thread(pluss::make_series({10, 7, 12}))},
});

auto first = frame.case_study().at(0);
std::cout << first;
```

### Output

```text
===================
     | city   units
-------------------
0    | Accra  10   
-------------------
[1 cs x 2 feat]
```

Example 2. A negative position counts from the end; `-1` is the last case. Chaining
`features(...)` afterward projects down to a single column.

```cpp
auto last = frame.case_study().at(-1).features({"city"});
std::cout << last;
```

### Output

```text
=============
     | city  
-------------
2    | Kumasi
-------------
[1 cs x 1 feat]
```

Example 3. An out-of-range position fails rather than clamping.

```cpp
try {
  (void)frame.case_study().at(3);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
DataFrame: row index out of bounds
```

## Notes

- `at()` never clamps. Compare with [`slice()`](slice.md), whose bounds are always
  normalized into range instead of throwing.
- The case label at the selected position is preserved on the result, so a one-row
  frame from `at()` still reports its original case identity, not a synthetic `0`.
- `at()` is the scalar-position sibling of [`take()`](take.md) (several positions) and
  [`slice()`](slice.md) (one contiguous interval).

## See Also

- [`DataFrameCaseStudyAccessor::slice`](slice.md): a contiguous positional interval,
  with clamped rather than validated bounds.
- [`DataFrameCaseStudyAccessor::take`](take.md): several explicit positions, gathered
  in request order.
- [`DataFrameCaseStudyAccessor::labels`](labels.md): select by case-axis value instead
  of physical position.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="labels.html"><span class="label">Next</span><span class="title">labels</span></a>
</div>
