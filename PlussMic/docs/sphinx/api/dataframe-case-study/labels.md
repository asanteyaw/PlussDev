<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 96bf81a537f1 -->

# DataFrameCaseStudyAccessor::labels

[Back to DataFrameCaseStudyAccessor API Reference](../../dataframe_case_study_accessor_api_reference)

## Signatures

```cpp
DataFrame DataFrameCaseStudyAccessor::labels(std::string_view value) const
DataFrame DataFrameCaseStudyAccessor::labels(std::int64_t value) const
DataFrame DataFrameCaseStudyAccessor::labels(Integer value) const  // any other integral type
DataFrame DataFrameCaseStudyAccessor::labels(double value) const
DataFrame DataFrameCaseStudyAccessor::labels(std::initializer_list<std::string_view> values) const
DataFrame DataFrameCaseStudyAccessor::labels(pluss::RangeView<std::string_view> values) const
DataFrame DataFrameCaseStudyAccessor::labels(std::initializer_list<std::int64_t> values) const
DataFrame DataFrameCaseStudyAccessor::labels(pluss::RangeView<std::int64_t> values) const
DataFrame DataFrameCaseStudyAccessor::labels(const Thread& values) const
```

## What It Does

Selects cases by case-axis label. A single scalar (`value`) selects every case whose
current case-axis label equals it, in source order. A list, range, or `Thread`
(`values`) performs several label lookups in one ordered selection, where the output
follows the requested order, not source order. Labels are lookup keys, not uniqueness
declarations: duplicate labels, or a label requested more than once, always expand to
every matching row. Numeric overloads use the same public label formatting used when
numeric case axes are rendered, allowing callers to stay in their native scalar type.
Every requested label must exist; the method never silently drops unknown keys or
returns an empty result in their place.

Matching is exact after the scalar is converted to the case axis's public label
representation. It is not a substring search, numeric tolerance comparison, or
hierarchical level match. Use `pick()` when one named level of a structured case axis
is the intended key.

The range overloads avoid an intermediate owning vector, while the `Thread` overload
is useful when the requested keys are already a PlussMic value. An empty multi-key
request is valid and returns a zero-case frame with the original feature schema.

## Parameters

**value** (`std::string_view` / `std::int64_t` / any other integral type / `double`)
    A single case-label key to match. Integral types other than `std::int64_t` convert
    implicitly through a templated overload. Its C++ type does not have to match the
    case axis's own dtype exactly (a `double` key still matches an `Int64` axis, for
    example), since comparison happens against the axis's rendered label text.

**values** (`std::initializer_list<std::string_view>` / `pluss::RangeView<std::string_view>`
/ `std::initializer_list<std::int64_t>` / `pluss::RangeView<std::int64_t>` / `const Thread&`)
    The ordered sequence of case-label keys to select. Order is significant: the
    result follows `values`' order, not the source frame's. The `RangeView` overloads
    borrow their argument — it must remain alive for the duration of the call.

## Returns

`DataFrame`
    A new frame containing every case matching the requested label(s). For the single
    `value` overloads, matches are in source order. For the multi-key `values`
    overloads, the result has one row per requested label per match, in requested
    order. Feature order, dtypes, title, and case labels are preserved. An empty
    `values` returns a valid zero-case frame with the original feature schema.

## Examples

Example 1. A single string label selects every matching case, in source order;
duplicate labels in the source frame return every match.

```cpp
using namespace pluss::table;
auto frame = DataFrame({{"v", Thread(pluss::make_series({3, 5, 8, 13}))}})
    .label().set(Thread(pluss::make_note({"A", "B", "B", "D"})));

auto duplicates = frame.case_study().labels("B");
std::cout << duplicates;
```

### Output

```text
===========
     | v   
-----------
B    | 5   
B    | 8   
-----------
[2 cs x 1 feat]
```

Example 2. A numeric case axis accepts a single `std::int64_t` key directly, without
first converting it to a string.

```cpp
auto numbered = DataFrame({{"v", Thread(pluss::make_series({4, 9, 16}))}})
    .label().set(Thread(pluss::make_series({10, 20, 30})));
auto by_int = numbered.case_study().labels(20);
std::cout << by_int;
```

### Output

```text
===========
     | v   
-----------
20   | 9   
-----------
[1 cs x 1 feat]
```

Example 3. Any other integral type, such as `int32_t`, reaches the same lookup through
the templated overload, which forwards to the `std::int64_t` form.

```cpp
std::int32_t key = 30;
auto by_int32 = numbered.case_study().labels(key);
std::cout << by_int32;
```

### Output

```text
===========
     | v   
-----------
30   | 16  
-----------
[1 cs x 1 feat]
```

Example 4. Floating-point labels use the same concise, twelve-significant-digit
rendering as the case axis itself.

```cpp
auto decimal = DataFrame({{"v", Thread(pluss::make_series({2, 4, 8}))}})
    .label().set(Thread(pluss::make_series({0.5, 1.25, 2.5})));
auto by_double = decimal.case_study().labels(1.25);
std::cout << by_double;
```

### Output

```text
===========
     | v   
-----------
1.25 | 4   
-----------
[1 cs x 1 feat]
```

Example 5. An absent label is an error, not an empty result — true for the single-value
overloads just as it is for the multi-key form shown later.

```cpp
try {
  (void)frame.case_study().labels("missing");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
DataFrame::case_study: case label not found 'missing'
```

Example 6. The result follows requested order, not source order.

```cpp
auto reordered = frame.case_study().labels({"D", "A"});
std::cout << reordered;
```

### Output

```text
===========
     | v   
-----------
D    | 13  
A    | 3   
-----------
[2 cs x 1 feat]
```

Example 7. A duplicate label in the source frame, requested once, still expands to
every matching row; requesting a label twice duplicates its matches again.

```cpp
auto expanded = frame.case_study().labels({"B", "A", "B"});
std::cout << expanded;
```

### Output

```text
===========
     | v   
-----------
B    | 5   
B    | 8   
A    | 3   
B    | 5   
B    | 8   
-----------
[5 cs x 1 feat]
```

Example 8. The `Thread` overload is useful when the requested keys are already a
PlussMic value, such as the output of another table operation.

```cpp
Thread keys(pluss::make_note({"A", "D"}));
auto from_thread = frame.case_study().labels(keys).features({"v"});
std::cout << from_thread;
```

### Output

```text
===========
     | v   
-----------
A    | 3   
D    | 13  
-----------
[2 cs x 1 feat]
```

Example 9. The borrowed-range overload is useful for a dynamically assembled request.
The range must remain alive for the duration of the call; the result does not borrow
it afterward.

```cpp
std::vector<std::string_view> wanted{"D", "B"};
auto from_range = frame.case_study().labels(
    pluss::RangeView<std::string_view>(wanted));
std::cout << from_range;
```

### Output

```text
===========
     | v   
-----------
D    | 13  
B    | 5   
B    | 8   
-----------
[3 cs x 1 feat]
```

Example 10. Integer label requests use the matching initializer-list overload.

```cpp
auto scaled = DataFrame({{"v", Thread(pluss::make_series({10, 20, 30}))}})
    .label().set(Thread(pluss::make_series({100, 200, 300})));
auto numeric = scaled.case_study().labels({300, 100});
std::cout << numeric;
```

### Output

```text
===========
     | v   
-----------
300  | 30  
100  | 10  
-----------
[2 cs x 1 feat]
```

Example 11. Integer label requests also have a borrowed-range form.

```cpp
std::vector<std::int64_t> numeric_keys{200, 100};
auto numeric_from_range = scaled.case_study().labels(
    pluss::RangeView<std::int64_t>(numeric_keys));
std::cout << numeric_from_range;
```

### Output

```text
===========
     | v   
-----------
200  | 20  
100  | 10  
-----------
[2 cs x 1 feat]
```

Example 12. An empty multi-key request is valid and returns a zero-case frame with the
original feature schema.

```cpp
auto empty = frame.case_study().labels(pluss::RangeView<std::string_view>{});
std::cout << "shape=" << empty.ncases() << "x" << empty.nfeatures() << "\n";
```

### Output

```text
shape=0x1
```

Example 13. Every requested label must exist in the multi-key form too; an unknown key
fails the whole request rather than silently dropping it.

```cpp
try {
  (void)frame.case_study().labels({"A", "missing"});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
DataFrame::case_study: case label not found 'missing'
```

## Notes

- A single `value` matches in source order; a multi-key `values` request follows the
  order of `values`, never source order — reorder a frame by case label by listing the
  labels in the desired order.
- A label requested more than once, or a label with duplicates in the source frame,
  expands fully every time; `labels()` never deduplicates.
- An unknown key fails the entire call; no partial result is returned, for either the
  single-value or multi-key overloads.
- Numeric overloads compare against the case axis's own public rendering, so `20` and
  `20.0` reach the same row on an `Int64` axis.

## See Also

- [`DataFrameCaseStudyAccessor::span`](span.md): a contiguous inclusive interval
  between two case-label endpoints, instead of an explicit list.
- [`DataFrameCaseStudyAccessor::take`](take.md): the positional equivalent — explicit
  row positions instead of case labels.
- [`DataFrameCaseStudyAccessor::pick`](pick.md): select by one named level of a
  structured, multi-level case axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="at.html"><span class="label">Previous</span><span class="title">at</span></a>
  <a class="next-link" href="pick.html"><span class="label">Next</span><span class="title">pick</span></a>
</div>
