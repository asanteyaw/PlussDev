<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 41c33d753264 -->

# DataFrameCaseStudyAccessor::take

[Back to DataFrameCaseStudyAccessor API Reference](../../dataframe_case_study_accessor_api_reference)

## Signatures

```cpp
DataFrame DataFrameCaseStudyAccessor::take(std::initializer_list<std::int64_t> rows) const
DataFrame DataFrameCaseStudyAccessor::take(pluss::RangeView<std::int64_t> rows) const
```

## What It Does

Gathers cases at explicit positions. Output order is exactly request order, repeated
positions produce repeated rows, and negative positions count from the end. Unlike
`slice()`, each position is validated independently and an out-of-range value fails
the complete operation before a result is returned.

Use the initializer-list overload for a small selection written at the call site. Use
the range overload when a ranking, search, or prior computation already produced the
positions. `take()` preserves every feature, dtype, selected case label, structured
case-axis level, and the frame title; chain `features(...)` afterward when the result
also needs feature projection.

Gathering is proportional to the number of requested rows across the frame's
features. It allocates a new result and does not mutate or reorder the source.

## Parameters

**rows** (`std::initializer_list<std::int64_t>` / `pluss::RangeView<std::int64_t>`)
    The ordered sequence of case positions to gather. Nonnegative values are physical
    positions; negative values count backward from `ncases()`. Each position is
    validated independently — a single out-of-range value fails the entire call. The
    `RangeView` overload borrows its argument, which must remain alive for the
    duration of the call.

## Returns

`DataFrame`
    A new frame with one row per requested position, in request order. Every feature,
    dtype, selected case label, structured case-axis level, and the frame title are
    preserved.

## Examples

Example 1. Output order is exactly request order, not source order.

```cpp
using namespace pluss::table;
auto frame = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"}))},
    {"units", Slate(pluss::make_series({10, 7, 12, 5}))},
});

auto reordered = frame.case_study().take({3, 0, 2});
std::cout << reordered;
```

### Output

```text
====================
     | city    units
--------------------
3    | Tamale  5    
0    | Accra   10   
2    | Kumasi  12   
--------------------
[3 cs x 2 feat]
```

Example 2. Duplicates and negative positions remain part of the request rather than
being canonicalized away.

```cpp
auto repeated = frame.case_study().take({-1, 1, -1});
std::cout << repeated;
```

### Output

```text
====================
     | city    units
--------------------
3    | Tamale  5    
1    | Tema    7    
3    | Tamale  5    
--------------------
[3 cs x 2 feat]
```

Example 3. The range overload accepts a reusable position buffer without forcing a
temporary initializer list.

```cpp
std::vector<std::int64_t> rows{2, 0};
auto selected = frame.case_study().take(pluss::RangeView<std::int64_t>(rows))
                     .features({"units"});
std::cout << selected;
```

### Output

```text
============
     | units
------------
2    | 12   
0    | 10   
------------
[2 cs x 1 feat]
```

Example 4. An empty position list creates a zero-case frame while preserving the
feature schema.

```cpp
auto empty = frame.case_study().take({});
std::cout << "ncases=" << empty.ncases() << " nfeatures=" << empty.nfeatures() << "\n";
```

### Output

```text
ncases=0 nfeatures=2
```

Example 5. Invalid positions do not clamp as `slice()` bounds do — a single
out-of-range value fails the whole request.

```cpp
try {
  (void)frame.case_study().take({0, 4});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
DataFrame: row index out of bounds
```

## Notes

- Every position is validated independently, unlike [`slice()`](slice.md)'s clamped
  bounds — a single out-of-range value fails the entire call before any row is
  gathered.
- Output order is exactly request order; duplicate positions produce duplicate output
  rows, and nothing is deduplicated or reordered.
- `take()` never mutates or reorders the source frame; it always allocates a new
  result.

## See Also

- [`DataFrameCaseStudyAccessor::at`](at.md): the single-position form of the same
  validated-position contract.
- [`DataFrameCaseStudyAccessor::slice`](slice.md): a contiguous interval with clamped,
  never-throwing bounds, instead of individually validated positions.
- [`DataFrameCaseStudyAccessor::labels`](labels.md): the label-oriented equivalent —
  an ordered list of case-label keys instead of positions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="span.html"><span class="label">Previous</span><span class="title">span</span></a>
  <a class="next-link" href="where.html"><span class="label">Next</span><span class="title">where</span></a>
</div>
