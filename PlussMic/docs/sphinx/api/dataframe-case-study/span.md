<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b82f4722ff8e -->

# DataFrameCaseStudyAccessor::span

[Back to DataFrameCaseStudyAccessor API Reference](../../dataframe_case_study_accessor_api_reference)

## Signatures

```cpp
DataFrame DataFrameCaseStudyAccessor::span(std::string_view start, std::string_view stop) const
DataFrame DataFrameCaseStudyAccessor::span(std::int64_t start, std::int64_t stop) const
DataFrame DataFrameCaseStudyAccessor::span(double start, double stop) const
DataFrame DataFrameCaseStudyAccessor::span(const Slate& start, const Slate& stop) const
```

## What It Does

Selects an inclusive interval on the current case-label axis. The first occurrence of
`start` opens the interval; the last occurrence of `stop` closes it. Every physical
row between those two positions is retained, whether or not its own label equals an
endpoint. This makes `span()` a boundary lookup followed by contiguous row selection,
not a lexical or numeric comparison between labels.

Duplicate endpoint labels are intentionally significant. A repeated start begins at
its first occurrence, while a repeated stop extends through its last occurrence. If
the stop lies before the start, the result is an empty frame. If either endpoint is
absent, the call fails rather than guessing an insertion position.

The numeric overloads format keys exactly as public numeric case labels are rendered.
The `Slate` overload accepts one scalar value per endpoint and is useful when bounds
already come from table operations; each endpoint Slate must be defined, strictly 1D,
and contain exactly one value.

## Parameters

**start** / **stop** (`std::string_view` / `std::int64_t` / `double` / `const Slate&`)
    The inclusive interval endpoints, in the case axis's own key representation. The
    `Slate` overload requires each argument to be a defined, 1D, single-element
    slate (`pluss::make_note({"X"})`, not the scalar `pluss::make_note("X")` — the
    latter constructs a 0D payload that `Slate`'s own constructor rejects).

## Returns

`DataFrame`
    A new frame containing every case from the first `start` through the last `stop`,
    inclusive. A reversed interval (`stop` before `start`) yields a valid, empty frame
    with the original feature schema rather than throwing.

## Examples

Example 1. A plain label interval; both endpoints are included.

```cpp
using namespace pluss::table;
auto frame = DataFrame({{"v", Slate(pluss::make_series({1, 2, 3, 4, 5}))}})
    .label().set(Slate(pluss::make_note({"A", "B", "B", "C", "D"})));

auto window = frame.case_study().span("B", "C");
std::cout << window;
```

### Output

```text
===========
     | v   
-----------
B    | 2   
B    | 3   
C    | 4   
-----------
[3 cs x 1 feat]
```

Example 2. The stop endpoint is inclusive, and duplicate starts widen the interval
from the first matching row — `"B"` to `"B"` keeps both `B` rows, not zero.

```cpp
auto all_b = frame.case_study().span("B", "B");
std::cout << all_b;
```

### Output

```text
===========
     | v   
-----------
B    | 2   
B    | 3   
-----------
[2 cs x 1 feat]
```

Example 3. Numeric labels can be passed without first converting them to strings.

```cpp
auto numbered = DataFrame({{"v", Slate(pluss::make_series({4, 9, 16, 25}))}})
    .label().set(Slate(pluss::make_series({10, 20, 30, 40})));
auto numeric = numbered.case_study().span(20, 30);
std::cout << numeric;
```

### Output

```text
===========
     | v   
-----------
20   | 9   
30   | 16  
-----------
[2 cs x 1 feat]
```

Example 4. Scalar Slates let computed bounds flow into the selection without leaving
the PlussMic value system. Each endpoint must be a strictly 1D, single-element Note —
constructed with a one-element initializer list, not the bare-scalar overload.

```cpp
Slate begin(pluss::make_note({"A"}));
Slate end(pluss::make_note({"C"}));
auto computed = frame.case_study().span(begin, end).features({"v"});
std::cout << computed;
```

### Output

```text
===========
     | v   
-----------
A    | 1   
B    | 2   
B    | 3   
C    | 4   
-----------
[4 cs x 1 feat]
```

Example 5. A reversed interval is valid and empty.

```cpp
auto reversed = frame.case_study().span("D", "A");
std::cout << "ncases=" << reversed.ncases() << "\n";
```

### Output

```text
ncases=0
```

Example 6. An unknown endpoint is not valid — both boundary labels must exist.

```cpp
try {
  (void)frame.case_study().span("A", "missing");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
DataFrame::case_study: case label not found 'missing'
```

## Notes

- `span()` is a boundary lookup followed by contiguous row selection, not a lexical or
  numeric comparison — rows between the endpoints are retained regardless of their own
  label, and label ordering in the source frame (not sorted order) determines which
  physical rows fall "between" the endpoints.
- A reversed interval returns a valid, empty frame; an absent endpoint throws instead
  of guessing an insertion position — the two failure modes are deliberately
  different.
- `pluss::make_note("X")` (a single string argument, no braces) constructs a scalar
  (0D) Note. The `Slate` overload's endpoints require a strictly 1D payload, so use
  `pluss::make_note({"X"})` instead — a one-element initializer list — when building a
  `Slate` endpoint directly.

## Choosing Between `span()` And `slice()`

`slice(1, 4)` means positions 1, 2, and 3. `span("B", "D")` means every row from
the first `B` through the last `D`, including both endpoints. Use `slice()` for stable
physical positions and `span()` when case labels are the durable contract.

## See Also

- [`DataFrameCaseStudyAccessor::slice`](slice.md): the positional, half-open
  counterpart to this label-oriented, inclusive interval.
- [`DataFrameCaseStudyAccessor::labels`](labels.md): exact label matches instead of an
  interval between two endpoints.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html"><span class="label">Previous</span><span class="title">slice</span></a>
  <a class="next-link" href="take.html"><span class="label">Next</span><span class="title">take</span></a>
</div>
