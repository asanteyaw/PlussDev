<!-- pluss-hand-authored -->

# ShelfCastAccessor::series

[Back to ShelfCastAccessor API Reference](../../shelf_cast_accessor_api_reference)

## Signature

```cpp
Shelf ShelfCastAccessor::series() const
```

## What It Does

Converts the bound shelf to a `Series`-kind shelf, that is, to numbers:

- a `Series` shelf is returned unchanged, or cast to `options.series_dtype` if that differs;
- a `Timeline` shelf becomes its epoch-nanosecond ticks as `Int64`;
- a `Duration` shelf becomes its signed nanosecond counts as `Int64`;
- a `Note` shelf is parsed as numbers (`Float64` by default, or `Bool` when `series_dtype` is `Bool`).

`options.series_dtype` then converts the result to the dtype you ask for.

## Parameters

The accessor takes the `ShelfCastOptions` supplied to [`Shelf::cast`](../shelf/cast.md). Only
`series_dtype` is used here.

## Returns

`Shelf`
    A new `Series`-kind shelf with the same length as the bound shelf. A `Note` value that cannot be parsed
    becomes `NULL`, whatever `options.invalid` says.

## Examples

Example 1. Parse numeric text with the default `Float64` dtype.

```cpp
using namespace pluss::table;

auto texts = Shelf(pluss::make_note({"1.0", "2.0", "3.0"}));

std::cout << texts.cast().series();
```

### Output

```text
0  1
1  2
2  3
ptype: Series
```

Example 2. Ask for a specific dtype.

```cpp
ShelfCastOptions options;
options.series_dtype = pluss::codem::ScalarType::Int64;

auto out = Shelf(pluss::make_note({"1", "2", "3"})).cast(options).series();
std::cout << out << "\n";
std::cout << out.dtype();
```

### Output

```text
0  1
1  2
2  3
ptype: Series

int64
```

Example 3. Text that is not a number becomes `NULL`.

```cpp
auto messy = Shelf(pluss::make_note({"4.0", "oops", "6.0"}));

std::cout << messy.cast().series();
```

### Output

```text
0  4
1  NULL
2  6
ptype: Series
```

Example 4. A `Timeline` becomes epoch nanoseconds, and a `Duration` becomes signed nanoseconds.

```cpp
auto days = Shelf(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"}));
auto lengths = Shelf(pluss::duration::nanos(std::vector<std::int64_t>{1500000000, -2000000000}, {2}));

std::cout << days.cast().series();
std::cout << lengths.cast().series();
```

### Output

```text
0  1704067200000000000
1  1704153600000000000
ptype: Series
0  1500000000
1  -2000000000
ptype: Series
```

## Notes

- Converting a shelf that is already a `Series` of the requested dtype is a cheap identity.
- `Shelf::astype(dtype)` is the shorthand for `cast(options).series()` with only the dtype set.

## See Also

- [`Shelf::cast`](../shelf/cast.md): enters the accessor and supplies the `ShelfCastOptions`.
- [`Shelf::astype`](../shelf/astype.md): the dtype-only shorthand.
- [`ShelfCastAccessor::note`](note.md), [`timeline`](timeline.md),
  [`duration`](duration.md): the other targets.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
  <a class="next-link" href="timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
