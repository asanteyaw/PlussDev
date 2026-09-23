<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 09fa421b17dd -->

# DataFrame::cast

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::cast() const -> DataFrameCastAccessor
DataFrame::cast(const ThreadCastOptions& options) const -> DataFrameCastAccessor
```

## What It Does

Enters a dedicated casting accessor bound to the current frame or subframe. `cast()`
itself does not convert anything yet; it stores shared `ThreadCastOptions`, and the
actual conversion happens when you finish the chain with one of the accessor methods:

- `series()`: cast every bound feature to `Series`
- `note()`: cast every bound feature to `Note`
- `timeline()`: cast every bound feature to `Timeline`
- `timeindex()`: cast every bound feature to `TimeIndex`

This is meant to be the ergonomic public surface for the common pattern "I already
selected the columns I care about; now cast all of them the same way":

```cpp
auto opened = df.features({"date", "birth_date"})
                 .cast({.format = "%Y-%m-%d", .invalid = ParseInvalid::Null})
                 .timeline();
```

That is the public casting contract for dataframe features.

Every method on the accessor has its own dedicated page — start at the
[DataFrameCastAccessor API Reference](../../dataframe_cast_accessor_api_reference)
and use its left sidebar to jump straight to `series`, `note`, `timeline`, or
`timeindex`.

## Parameters

**options** (`const ThreadCastOptions&`, overload)

Shared cast controls for every feature currently bound on the frame:

- `series_dtype`: target numeric `ScalarType` when the target is `Series`
- `format`: date/time parse pattern for `Timeline` or `TimeIndex` targets
- `invalid`: `ParseInvalid::Raise` or `ParseInvalid::Null` for unparseable date text

The zero-argument overload is shorthand for default `ThreadCastOptions{}`.

## Returns

`DataFrameCastAccessor`
    A lightweight accessor bound to the current frame. Each accessor method returns
    a new `DataFrame`; the source frame is never mutated.

## Examples

Example 1. Cast two selected text date columns together without repeating their names
inside `CastSpec`s.

```cpp
using namespace pluss::table;

auto raw = DataFrame({
    {"opened_at", Thread(pluss::make_note({"2024-01-01 08:00:00", "bad-date"}))},
    {"closed_at", Thread(pluss::make_note({"2024-01-02 09:30:00", "2024-01-03 10:45:00"}))},
    {"city",      Thread(pluss::make_note({"Accra", "Tema"}))},
});

ThreadCastOptions options;
options.format = "%Y-%m-%d %H:%M:%S";
options.invalid = ParseInvalid::Null;

auto temporal = raw.features({"opened_at", "closed_at"}).cast(options).timeline();
std::cout << temporal;
```

### Output

```text
========================================
     | opened_at            closed_at   
----------------------------------------
0    | 2024-01-01 08:00:00  2024-01-02 09:30:00
1    | NULL                 2024-01-03 10:45:00
----------------------------------------
[2 cs x 2 feat]
```

Both selected features are cast with one shared `format` and one shared
`invalid = Null` policy. `city` is untouched because it was never part of the
selected subframe.

Example 2. Parse one selected text column into numeric `Series`.

```cpp
ThreadCastOptions numeric;
numeric.series_dtype = pluss::codem::ScalarType::Float64;

auto amounts = DataFrame({
    {"amount_text", Thread(pluss::make_note({"10.5", "oops"}))},
    {"city",        Thread(pluss::make_note({"Accra", "Tema"}))},
});

auto parsed = amounts.features({"amount_text"}).cast(numeric).series();
std::cout << parsed;
```

### Output

```text
==================
     | amount_text
------------------
0    | 10.5       
1    | NULL       
------------------
[2 cs x 1 feat]
```

Only the selected `amount_text` feature is cast. `"oops"` fails numeric parsing and
becomes `NULL`, matching the normal `Note -> Series` conversion path.

## Notes

- `cast()` does not select columns. Selection belongs on [`features(names)`](features.md).
- If you need to preserve the full frame while replacing only one or more casted
  columns, compose `features(...).cast()` with [`set(...)`](set.md).
- The accessor applies one shared `ThreadCastOptions` scribble to every currently bound
  feature.
- Calling `cast()` on a zero-feature frame is valid; the accessor methods simply
  return the same zero-feature frame.

## See Also

- [`DataFrame::features`](features.md): the most natural way to scope which columns
  the cast accessor should operate on.
- [`DataFrame::astype`](astype.md): the dtype-only shorthand when the target is
  definitely `Series`.
- [`DataFrameLabelAccessor::values`](../dataframe-label/values.md): pull the case-label
  thread when you need to cast labels explicitly and then reattach them with
  [`set(...)`](../dataframe-label/set.md).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="case_study.html">
    <span class="label">Previous</span>
    <span class="title">case_study</span>
  </a>
  <a class="next-link" href="clip.html">
    <span class="label">Next</span>
    <span class="title">clip</span>
  </a>
</div>
