<!-- pluss-hand-authored -->

# DataFrameCastAccessor::duration

[Back to DataFrameCastAccessor API Reference](../../dataframe_cast_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameCastAccessor::duration() const
```

## What It Does

Casts every feature currently bound on the accessor to `Duration` (signed nanoseconds), using the shared
`ShelfCastOptions` from `DataFrame::cast(...)`. Each feature follows the rules of
[`ShelfCastAccessor::duration`](../shelf-cast/duration.md): an `Int64` series is read as nanoseconds, and a
`Note` must hold exact signed nanoseconds as decimal text.

## Returns

`DataFrame`
    A new frame with the same feature names and case labels, with every bound feature converted to
    `Duration`.

## Examples

Example 1. Cast an `Int64` column and a text column together.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"ns",  Shelf(pluss::make_series(std::vector<std::int64_t>{1500000000, 60000000000}, {2}))},
    {"txt", Shelf(pluss::make_note({"2000000000", "-5"}))},
});

std::cout << df.cast().duration();
```

### Output

```text
==============================
     | ns      txt            
------------------------------
0    | PT1.5S  PT2S           
1    | PT1M    -PT0.000000005S
------------------------------
[2 cs x 2 feat]
```

Example 2. A text value that is not an exact nanosecond count becomes `NULL` when `invalid` is `Null`.

```cpp
ShelfCastOptions options;
options.invalid = ParseInvalid::Null;

auto messy = DataFrame({{"txt", Shelf(pluss::make_note({"2000000000", "soon"}))}});

std::cout << messy.cast(options).duration();
```

### Output

```text
===========
     | txt 
-----------
0    | PT2S
1    | NULL
-----------
[2 cs x 1 feat]
```

Example 3. A floating-point column is refused.

```cpp
auto floats = DataFrame({{"x", Shelf(pluss::make_series({1.5, 2.5}))}});

try {
  auto out = floats.cast().duration();
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
Shelf::cast(Duration): Series source must have Int64 dtype
```

## Notes

- `options.format` is not accepted for `Duration` and throws.

## See Also

- [`DataFrame::cast`](../dataframe/cast.md): enters the accessor and supplies shared `ShelfCastOptions`.
- [`DataFrameCastAccessor::timeline`](timeline.md): the point-in-time target.
