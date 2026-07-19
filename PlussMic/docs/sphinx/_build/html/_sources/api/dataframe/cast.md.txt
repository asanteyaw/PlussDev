<!-- pluss-hand-authored -->

# DataFrame::cast

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::cast(pluss::msharp::RangeView::Of<CastSpec> specs) const -> DataFrame
DataFrame::cast(std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const -> DataFrame
DataFrame::cast(pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const -> DataFrame
DataFrame::cast(std::initializer_list<std::pair<std::string_view, ThreadCastTarget>> feature_targets) const -> DataFrame
DataFrame::cast(pluss::msharp::RangeView::Of<std::pair<std::string_view, ThreadCastTarget>> feature_targets) const -> DataFrame
```

## What It Does

Converts selected features to a different scalar dtype, or to an entirely different
payload kind (`Series` numeric, `Note` text, `Timeline`/`TimeIndex` calendar), while
leaving every other feature untouched. There are three ways to call it, in order of
increasing power:

- **dtype-only** (`(name, ScalarType)` pairs) — the same as [`astype()`](astype.md):
  recast a numeric `Series` feature to a different concrete scalar type (`Int64`,
  `Float64`, `Bool`, and so on) without changing its payload kind.
- **cross-kind** (`(name, ThreadCastTarget)` pairs) — convert a feature to a different
  payload kind entirely (`Series`, `Note`, `Timeline`, or `TimeIndex`), using default
  parse/format behavior.
- **full control** (`RangeView<CastSpec>`) — the same cross-kind conversion, but with
  an explicit `ThreadCastOptions` per feature: a target `series_dtype`, a date
  `format` string, and an `invalid` policy for unparseable text.

When two entries in one call target the same feature name, only the first is applied
and every subsequent one is silently skipped — not an error, just a quiet no-op for
the duplicate.

## Parameters

**specs** (`pluss::RangeView<CastSpec>`)

`CastSpec{feature, target, options}`: `feature` is the column to convert (must exist;
throws `"DataFrame::cast: unknown feature '<name>'"` otherwise), `target`
(`ThreadCastTarget`, default `Series`) is the destination payload kind, and `options`
(`ThreadCastOptions`) carries:

- **`series_dtype`** (`std::optional<ScalarType>`): when casting *to* `Series`, the
  concrete numeric type to land on (defaults to `Float64` when converting from
  `Note`/`Timeline`/`TimeIndex` text or ticks, or left unchanged when the source is
  already `Series`).
- **`format`** (`std::string`): an explicit date-format string, consulted only when
  casting `Note` text to `Timeline`/`TimeIndex`.
- **`invalid`** (`ParseInvalid`, default `Raise`): what happens to a `Note` value that
  fails to parse as a date under `format` — `Raise` throws, `Null` produces a `NULL`
  cell instead.

**feature_dtypes** (pairs of `(std::string_view, ScalarType)`)

Recast named `Series` features to a different scalar dtype, exactly like
[`astype()`](astype.md).

**feature_targets** (pairs of `(std::string_view, ThreadCastTarget)`)

Convert named features to a different payload kind using default `ThreadCastOptions`
(`series_dtype` unset, no format string, `invalid = Raise`).

## Returns

`DataFrame`
    A new frame with the requested features cast; every untargeted feature is
    preserved unchanged.

## Examples

Example 1. Widen an `Int64`-looking numeric feature to `Float64` using the plain
dtype-pair overload.

```cpp
using namespace pluss::table;

auto students = DataFrame({
    {"id",   Thread(pluss::make_series({1, 2, 3}))},
    {"name", Thread(pluss::make_note({"Ama", "Kofi", "Efua"}))},
});

auto out = students.cast({{"id", pluss::codem::ScalarType::Float64}});
std::cout << out;
```

### Output

```text
=================
     | id    name
-----------------
0    | 1     Ama 
1    | 2     Kofi
2    | 3     Efua
-----------------
[3 cs x 2 feat]
```

`id`'s values look the same, but it is now backed by `Float64` rather than `Int64` —
useful before an operation that requires floating-point arithmetic (fractional
division, for instance) on a column that started out as whole numbers.

Example 2. Parse a `Note` column of numeric-looking strings into a real `Series`,
letting an unparseable entry become `NULL` instead of failing the whole call.

```cpp
auto raw = DataFrame({
    {"amount_text", Thread(pluss::make_note({"120", "85.5", "N/A"}))},
});

auto parsed = raw.cast({{"amount_text", ThreadCastTarget::Series}});
std::cout << parsed;
```

### Output

```text
==================
     | amount_text
------------------
0    | 120        
1    | 85.5       
2    | NULL       
------------------
[3 cs x 1 feat]
```

With no explicit `series_dtype`, a `Note` → `Series` cast defaults to `Float64`.
`"N/A"` doesn't parse as a number, so it silently becomes `NULL` — this particular
`Note` → `Series` path never throws on an unparseable value, unlike the `Note` →
`Timeline` path, whose behavior depends on the `invalid` option.

Example 3. Cast to `Bool` via a full `CastSpec`, and see which text tokens
`parse_bool_like` actually recognizes.

```cpp
DataFrame::CastSpec to_bool;
to_bool.feature = "flag_text";
to_bool.target = ThreadCastTarget::Series;
to_bool.options.series_dtype = pluss::codem::ScalarType::Bool;

auto flags = DataFrame({
    {"flag_text", Thread(pluss::make_note({"true", "false", "maybe"}))},
});

auto out = flags.cast({to_bool});
std::cout << out;
```

### Output

```text
================
     | flag_text
----------------
0    | true     
1    | false    
2    | NULL     
----------------
[3 cs x 1 feat]
```

`"true"`/`"false"` (also `"1"`/`"0"` and `"yes"`/`"no"`, case-insensitively) parse
cleanly; `"maybe"` matches none of them, so it becomes `NULL` rather than raising.

Example 4. `Note` → `Timeline` with an explicit `format`: an unparseable value
raises by default, but `invalid = Null` turns that into a `NULL` cell instead.

```cpp
auto dates = DataFrame({
    {"date_text", Thread(pluss::make_note({"2024-01-01", "not-a-date"}))},
});

DataFrame::CastSpec to_date;
to_date.feature = "date_text";
to_date.target = ThreadCastTarget::Timeline;
to_date.options.format = "%Y-%m-%d";

try {
  auto out = dates.cast({to_date});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

DataFrame::CastSpec to_date_null = to_date;
to_date_null.options.invalid = ParseInvalid::Null;

auto out2 = dates.cast({to_date_null});
std::cout << out2;
```

### Output (default invalid = Raise)

```text
Thread::cast(Timeline): could not parse datetime value 'not-a-date'
```

### Output (invalid = Null)

```text
=================
     | date_text 
-----------------
0    | 2024-01-01
1    | NULL      
-----------------
[2 cs x 1 feat]
```

With the default `invalid = ParseInvalid::Raise`, hitting `"not-a-date"`
against the `"%Y-%m-%d"` pattern throws immediately. Setting `invalid =
ParseInvalid::Null` on an otherwise-identical spec instead lets the whole
cast complete, turning just the unparseable row into `NULL` — the opposite
of `Note` → `Series`'s always-`NULL, never-raise behavior shown in Example 2.

Example 5. Name a feature that doesn't exist, to see the call throw.

```cpp
try {
  auto out = students.cast({{"not_a_column", pluss::codem::ScalarType::Float64}});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::cast: unknown feature 'not_a_column'
```

`students` only has `id` and `name` — every `cast()` overload validates every
named feature up front, before any conversion runs.

## Notes

- A duplicate feature name across multiple entries in one `cast()` call is not an
  error — only the first entry for that name is applied, and later ones are silently
  ignored.
- `Note` → `Series` never raises on unparseable text; it always produces `NULL`
  instead. `Note` → `Timeline`/`TimeIndex` is the opposite: it raises by default
  (`ParseInvalid::Raise`) unless you set `options.invalid = ParseInvalid::Null`.
- Casting a numeric `Series` to `Timeline` auto-detects whether the values look like
  an integer-encoded `YYYYMMDD` calendar date (every valid value in the range
  `10000101`–`99991231`) or like raw epoch-nanosecond ticks, and interprets the whole
  column accordingly — it does not support a mixed column.
- `feature_dtypes` and `astype()`'s equivalent overload are literally the same
  function under a different name — pick whichever reads better at the call site.

## See Also

- [`DataFrame::astype`](astype.md): the dtype-only alias for `cast()`'s
  `feature_dtypes` overload.
- [`DataFrame::invalid`](invalid.md): find cells that failed validation, separate from
  the parse-failure `NULL`s a cast can introduce.
- [`DataFrame::features`](features.md): extract a single already-cast column as a
  `Thread` for further inspection.

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
