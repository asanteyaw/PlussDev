<!-- pluss-hand-authored -->

# DataFrameLabelAccessor::cast

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signatures

```cpp
DataFrameLabelAccessor::cast(ThreadCastTarget target, const ThreadCastOptions& options = {}) const -> DataFrame
DataFrameLabelAccessor::cast(ThreadCastTarget target, std::optional<pluss::codem::ScalarType> series_dtype) const -> DataFrame
```

## What It Does

Casts the case labels to a different `ThreadCastTarget` payload family
(`Series`, `Note`, `Timeline`, or `TimeIndex`) — every structured level if the
frame has any, or the flat case-label thread otherwise. Feature columns are
untouched. Casting a numeric-looking `Note` axis (like case labels written as
`"1"`, `"2"`, `"10"`) to `Series` doesn't just relabel the same text — it
changes how later operations like [`sort()`](sort.md) or
[`monotonic()`](monotonic.md) compare rows, from lexicographic (string) order
to numeric order.

## Parameters

**target** (`ThreadCastTarget`)

The destination payload kind: `Series`, `Note`, `Timeline`, or `TimeIndex`.

**options** (`const ThreadCastOptions&`, one overload)

Full cast controls: `series_dtype` (target numeric `ScalarType` when casting to
`Series`), `format` (parse/format pattern for `Timeline`/`TimeIndex` targets),
and `invalid` (`ParseInvalid::Raise` or `::Null` for unparseable values).

**series_dtype** (`std::optional<pluss::codem::ScalarType>`, one overload)

Shorthand for `ThreadCastOptions{.series_dtype = series_dtype}` — the target
numeric dtype when casting to `Series`.

## Returns

`DataFrame`
    A new frame with the same features, and case labels cast to `target`.

## Examples

Example 1. Cast `Note` case labels holding digit strings to a real numeric
`Series`, and see the comparison semantics change.

```cpp
using namespace pluss::table;

auto ids = DataFrame(
    {{"value", Thread(pluss::make_series({200.0, 150.0, 100.0}))}},
    Thread(pluss::make_note({"10", "2", "1"})));
std::cout << ids;

auto lexicographic = ids.label().sort();
std::cout << lexicographic;
```

### Output (before)

```text
============
     | value
------------
10   | 200  
2    | 150  
1    | 100  
------------
[3 cs x 1 feat]
```

### Output (sort, before cast — lexicographic)

```text
============
     | value
------------
1    | 100  
10   | 200  
2    | 150  
------------
[3 cs x 1 feat]
```

As plain `Note` text, `"1" < "10" < "2"` — `"10"` sorts before `"2"`, since
string comparison looks at the first character first.

```cpp
auto casted = ids.label().cast(ThreadCastTarget::Series, pluss::codem::ScalarType::Int64);
auto numeric_sorted = casted.label().sort();
std::cout << numeric_sorted;
```

### Output (sort, after cast — numeric)

```text
============
     | value
------------
1    | 100  
2    | 150  
10   | 200  
------------
[3 cs x 1 feat]
```

Same three case labels, same digits displayed — but `casted`'s case labels are
now a genuine `Int64` `Series`, so `1 < 2 < 10` numerically, and `"10"` now
correctly sorts last.

Example 2. Use the `ThreadCastOptions` overload with `invalid =
ParseInvalid::Null` to turn an unparseable label into `NULL` instead of
throwing.

```cpp
using namespace pluss::table;

auto ids = DataFrame(
    {{"value", Thread(pluss::make_series({100.0, 150.0, 999.0}))}},
    Thread(pluss::make_note({"1", "2", "abc"})));
std::cout << ids;

ThreadCastOptions options;
options.series_dtype = pluss::codem::ScalarType::Int64;
options.invalid = ParseInvalid::Null;
auto out = ids.label().cast(ThreadCastTarget::Series, options);
std::cout << out;
```

### Output (before)

```text
============
     | value
------------
1    | 100  
2    | 150  
abc  | 999  
------------
[3 cs x 1 feat]
```

### Output (after)

```text
============
     | value
------------
1    | 100  
2    | 150  
NULL | 999  
------------
[3 cs x 1 feat]
```

`"1"` and `"2"` parse cleanly to `Int64`, but `"abc"` doesn't — with the
default `invalid = ParseInvalid::Raise` this would throw; with `Null`, that
one label becomes `NULL` instead, and the other two rows are unaffected.

## Notes

- Casting a structured (multi-level) frame casts **every** level to `target`.
- `Note` → `Series`/`Timeline`/`TimeIndex` casts can fail to parse; use
  `ThreadCastOptions{.invalid = ParseInvalid::Null}` to turn unparseable values
  into `NULL` instead of throwing.
- Because every column in the ASCII table renderer is left-justified regardless
  of its underlying type, a cast between text-like and numeric representations
  of the same digits is often invisible in the printed table — only later
  comparisons (`sort()`, `monotonic()`, `align()`) reveal the difference.

## See Also

- [`DataFrameLabelAccessor::sort`](sort.md): one of the operations whose
  behavior depends on whether case labels are numeric or textual.
- [`DataFrameNoteAccessor`](../dataframe_note_accessor_api_reference): the
  feature-column counterpart for casting/reformatting `Note` values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align.html">
    <span class="label">Previous</span>
    <span class="title">align</span>
  </a>
  <a class="next-link" href="codes.html">
    <span class="label">Next</span>
    <span class="title">codes</span>
  </a>
</div>
