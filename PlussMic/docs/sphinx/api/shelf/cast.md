<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 07df050f1cef -->

# Shelf::cast

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
ShelfCastAccessor Shelf::cast() const
ShelfCastAccessor Shelf::cast(const ShelfCastOptions& options) const
```

## What It Does

Enters the casting accessor — `cast()` itself performs no conversion; it binds the
current shelf (and, optionally, a `ShelfCastOptions` policy) and hands back a
`ShelfCastAccessor` whose four methods each convert to one target payload family:

### `ShelfCastAccessor` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `series` | Method | `Shelf series() const` |
| `note` | Method | `Shelf note() const` |
| `timeline` | Method | `Shelf timeline() const` |
| `duration` | Method | `Shelf duration() const` |

Calling the target that matches the source shelf's own [`kind()`](kind.md) is always a
cheap identity — `Shelf(...).cast().series()` on an already-`Series`-kind shelf just
returns `*this` unchanged (subject to `options.series_dtype`, if you're also asking for a
different numeric dtype). Converting *between* kinds follows these rules:

- **→ `series()`**: `Timeline` sources become their epoch-nanosecond integer ticks, and `Duration`
  sources their signed nanosecond counts, (as `Int64`, further converted to `options.series_dtype` if set). `Note` sources
  are parsed as numbers (`Float64` by default, or `Bool` if `series_dtype` is `Bool`,
  using permissive true/false-like text matching).
- **→ `note()`**: every other kind is formatted as text — `Timeline` as ISO strings, `Duration` as
  ISO-8601 durations, `Series` via each scalar's own text representation.
- **→ `timeline()`**: `Duration` sources are refused (elapsed time is not a timestamp). `Series` sources must be
  numeric and are interpreted either as epoch-nanosecond ticks or, when every valid value
  looks like an 8-digit `YYYYMMDD` integer, as calendar dates. `Note` sources are parsed as
  ISO-ish datetime text (or via `options.format`, if set).
- **→ `duration()`**: `Series` sources must have the `Int64` dtype and are read as signed nanoseconds. `Note`
  sources must hold exact signed nanoseconds as decimal text. `options.format` is not accepted.

### `ShelfCastOptions` fields

| Field | Type | Default |
|---|---|---|
| `series_dtype` | `std::optional<pluss::codem::ScalarType>` | `std::nullopt` |
| `format` | `std::string` | empty |
| `invalid` | `ParseInvalid` | `ParseInvalid::Raise` |

```{important} ShelfCastOptions::invalid only takes effect for the Timeline and Duration targets
`invalid` (`Raise` or `Null`) is meant to choose what happens when a `Note` value can't be
parsed. In the current implementation, that choice is only actually consulted by the
`Timeline` and `Duration` targets: `Raise` (the default) throws on the
first unparseable value there, `Null` marks it invalid instead. For the **`Series`**
target, unparseable `Note` text is **always** silently marked `NULL` — `invalid` is never
consulted at all, regardless of which value you set it to. See Examples 3 and 4 for both
sides of this.
```

## Parameters

**options** (`const ShelfCastOptions&`, optional)
    `series_dtype` picks the concrete numeric dtype for a `series()` conversion (defaults
    to `Float64`, or effectively `Bool` when you explicitly request it). `format`, if
    non-empty, is a strptime-style pattern used to parse `Note` text for a `timeline()`
    conversion instead of the default ISO-ish parser (it is rejected for `duration()`). `invalid` chooses
    `Raise`-vs-`Null` behavior — see the important callout above for exactly which target
    actually honors it.

## Returns

`ShelfCastAccessor`
    A lightweight accessor bound to the current shelf (and `options`, if given). Each of
    its four methods performs one conversion and returns a `Shelf`.

## Examples

Example 1. Parse numeric text into a `Series`, using the default `Float64` dtype.

```cpp
using namespace pluss::table;

auto texts = Shelf(pluss::make_note({"1.0", "2.0", "3.0"}));

auto out = texts.cast().series();
std::cout << out;
```

### Output

```text
0  1
1  2
2  3
ptype: Series
```

Example 2. Request a specific `series_dtype` via `ShelfCastOptions`.

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

Example 3. Unparseable `Note` text becomes `NULL` when casting to `Series` — this happens
**regardless** of `options.invalid`, even at its `Raise` default. See the important
callout above.

```cpp
auto messy = Shelf(pluss::make_note({"4.0", "oops", "6.0"}));

auto out = messy.cast().series();
std::cout << out;
```

### Output

```text
0  4
1  NULL
2  6
ptype: Series
```

Example 4. The same unparseable-text scenario, but casting to `Timeline` instead — here,
`options.invalid`'s default `Raise` actually does throw. The message comes from the
underlying datetime parser, not a `Shelf`-specific wrapper.

```cpp
auto bad_dates = Shelf(pluss::make_note({"2024-01-01T00:00:00Z", "not a date"}));
try {
  auto out = bad_dates.cast().timeline();
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}

ShelfCastOptions null_on_invalid;
null_on_invalid.invalid = ParseInvalid::Null;
auto out2 = bad_dates.cast(null_on_invalid).timeline();
std::cout << out2;
```

### Output

```text
moment: failed to parse datetime
0  2024-01-01
1  NULL
ptype: Timeline
```

Setting `invalid = ParseInvalid::Null` avoids the throw and marks the unparseable row
`NULL` instead.

Example 5. Cast a numeric shelf to text with `note()`.

```cpp
auto amounts = Shelf(pluss::make_series({1.5, 2.5}));

auto out = amounts.cast().note();
std::cout << out;
```

### Output

```text
0  1.5
1  2.5
ptype: Note
```

## Notes

- `cast()` alone performs no conversion — it only binds an accessor; the actual work
  happens in `.series()`/`.note()`/`.timeline()`/`.duration()`.
- `ShelfCastOptions::invalid` only actually affects the `Timeline` and `Duration` targets —
  see the important callout above. `astype()`, which delegates to `cast().series()`
  internally, inherits this same "always null on unparseable text" behavior.
- Converting to a shelf's own current kind is a cheap identity (subject to
  `series_dtype`, if a different numeric dtype is also requested).

## See Also

- [`Shelf::astype`](astype.md): the single-argument shorthand for
  `cast(options).series()`.
- [`Shelf::as_series`](as_series.md) / [`as_note`](as_note.md) /
  [`as_timeline`](as_timeline.md): view the current
  payload without converting it, throwing if the kind doesn't already match.
- [`DataFrame::cast`](../dataframe/cast.md): cast several frame features in one call.
- [`ShelfCastAccessor`](../../shelf_cast_accessor_api_reference.md): one page for each of the four targets.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="casefold.html">
    <span class="label">Previous</span>
    <span class="title">casefold</span>
  </a>
  <a class="next-link" href="chars.html">
    <span class="label">Next</span>
    <span class="title">chars</span>
  </a>
</div>
