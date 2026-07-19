<!-- pluss-hand-authored -->

# DataFrame::observe

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::observe(std::int64_t row, bool render = false) const -> DataFrameView
DataFrame::observe(std::int64_t start, std::int64_t stop, bool render = false) const -> DataFrameView
```

## What It Does

Previews a positional window of rows — either a single row, or a contiguous
`[start, stop)` range — without materializing a separate named helper call for each
shape. Internally, the single-row overload just calls the range overload with
`(row, row + 1)`, and the range overload is a thin wrapper around
[`slice()`](slice.md) that tags the resulting `DataFrameView` with the display
name `"observe"`. It exists alongside [`head()`](head.md) and [`tail()`](tail.md)
for the case those two don't cover: peeking at rows from the *middle* of a frame by
position.

## Parameters

**row** (`std::int64_t`)

Zero-based positional row to preview, for the single-row overload. Equivalent to
`observe(row, row + 1)`.

**start** / **stop** (`std::int64_t`)

The inclusive start / exclusive stop positions of the row window to preview, for the
range overload — identical slicing semantics to [`slice()`](slice.md).

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`, identical in behavior to
[`head()`](head.md)'s `render` parameter.

## Returns

`DataFrameView`
    A view over the selected row window, with the original feature set and the
    original case labels of the selected rows preserved (positions are not
    renumbered).

## Examples

Example 1. Preview a single row from the middle of a frame by position.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto middle_row = orders.observe(1);
std::cout << middle_row;
```

### Output

```text
========================
     | id    sales  city
------------------------
1    | 2     120    Tema
------------------------
[1 cs x 3 feat]
```

The result keeps case `1`'s original label (`"1"`) rather than renumbering it to
`"0"` — `observe()` previews by position, but the identity of the row it found is
still visible in the output.

Example 2. Preview a contiguous row window.

```cpp
auto first_two = orders.observe(0, 2);
std::cout << first_two;
```

### Output

```text
=========================
     | id    sales  city 
-------------------------
0    | 1     100    Accra
1    | 2     120    Tema 
-------------------------
[2 cs x 3 feat]
```

## Notes

- `observe(row)` is exactly `observe(row, row + 1)` — a one-row range under the hood,
  not a separate code path.
- Both overloads use positional (`0`-based) row coordinates, never case-label
  lookup — the resulting view's row keeps whatever case label it already had, but
  `row`/`start`/`stop` themselves always count by position.
- `render = true` allows the returned `DataFrameView` to auto-print in a temporary-value
  context, the same convention `head()` and `tail()` use.

## See Also

- [`DataFrame::head`](head.md), [`DataFrame::tail`](tail.md): the fixed-edge previews
  this method complements for middle-of-frame windows.
- [`DataFrame::slice`](slice.md): the underlying positional row-range primitive
  `observe(start, stop, ...)` wraps.
- [`DataFrame::filter`](filter.md): select rows by a boolean condition rather than by
  position.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="percentiles.html">
    <span class="label">Next</span>
    <span class="title">percentiles</span>
  </a>
</div>
