<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c3b538890444 -->

# DataFrameCaseStudyAccessor::slice

[Back to DataFrameCaseStudyAccessor API Reference](../../dataframe_case_study_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameCaseStudyAccessor::slice(std::int64_t start, std::int64_t stop) const
```

## What It Does

Selects one contiguous positional interval using the half-open range `[start, stop)`.
`start` is included and `stop` is excluded. Negative bounds count backward from the
case count, and both normalized bounds are clamped to `[0, ncases()]`. A reversed or
collapsed interval therefore returns a zero-case frame rather than throwing.

`slice()` is the right terminal when adjacency and position define the selection. It
does not inspect case labels. Use `span()` when the two boundaries are case-label
values instead; a label span is inclusive and follows different duplicate-label
rules.

The result owns the selected column values while retaining feature order, feature
dtypes, title, surviving case labels, and structured case-axis levels. The source
frame is unchanged. Work is linear in the number of selected values across all
features.

## Parameters

**start** / **stop** (`std::int64_t`)
    The half-open interval `[start, stop)`. Nonnegative values are physical
    positions; negative values count backward from `ncases()`. Both bounds are
    normalized, then clamped to `[0, ncases()]` — no combination of bounds throws.

## Returns

`DataFrame`
    A new frame containing the cases in `[start, stop)` after normalization and
    clamping. Feature order, dtypes, title, surviving case labels, and structured
    case-axis levels are preserved. A reversed or fully out-of-range interval yields a
    valid zero-case frame with the original feature schema.

## Examples

Example 1. A plain positional interval.

```cpp
using namespace pluss::table;
auto frame = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"}))},
    {"units", Slate(pluss::make_series({10, 7, 12, 5}))},
});

auto middle = frame.case_study().slice(1, 3);
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

Example 2. Negative bounds are normalized before clamping. Here `-3` and `-1` become
positions `1` and `3`, so the final case remains excluded — identical to Example 1 on
this four-case frame.

```cpp
auto from_end = frame.case_study().slice(-3, -1);
std::cout << from_end;
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

Example 3. Bounds beyond either edge are safe. Clamping makes this an explicit way to
request "from the beginning through at most two cases" without first reading
`ncases()`.

```cpp
auto prefix = frame.case_study().slice(-99, 2).features({"city"});
std::cout << prefix;
```

### Output

```text
============
     | city 
------------
0    | Accra
1    | Tema 
------------
[2 cs x 1 feat]
```

Example 4. A reversed interval remains a valid frame with the original feature
schema — this is different from [`at()`](at.md), where an invalid scalar position is
an error.

```cpp
auto empty = frame.case_study().slice(3, 1);
std::cout << "ncases=" << empty.ncases() << " nfeatures=" << empty.nfeatures() << "\n";
```

### Output

```text
ncases=0 nfeatures=2
```

## Notes

- `slice()` has no out-of-bounds failure because its bounds are deliberately clamped,
  never validated. Construction can still fail if the source frame itself is
  undefined or internally invalid.
- Use [`at()`](at.md) or [`take()`](take.md) when an invalid requested position should
  be treated as a programming error instead of an empty or clipped interval.
- `slice()` is purely positional; it never inspects case-axis values. Use
  [`span()`](span.md) when the two boundaries are case-label values instead — note
  that a label span is inclusive, unlike `slice()`'s half-open interval.

## See Also

- [`DataFrameCaseStudyAccessor::span`](span.md): the label-oriented, inclusive
  counterpart to this positional, half-open interval.
- [`DataFrameCaseStudyAccessor::at`](at.md): a single validated position instead of a
  clamped interval.
- [`DataFrameCaseStudyAccessor::take`](take.md): explicit, individually validated
  positions instead of one contiguous interval.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pick.html"><span class="label">Previous</span><span class="title">pick</span></a>
  <a class="next-link" href="span.html"><span class="label">Next</span><span class="title">span</span></a>
</div>
