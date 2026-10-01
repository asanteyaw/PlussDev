<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b5a8285603fe -->

# Thread::cast

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::cast() const -> ThreadCastAccessor
Thread::cast(const ThreadCastOptions& options) const -> ThreadCastAccessor
```

## What It Does

Enters the casting accessor — `cast()` itself performs no conversion; it binds the
current thread (and, optionally, a `ThreadCastOptions` policy) and hands back a
`ThreadCastAccessor` whose four methods each convert to one target payload family:

```cpp
class ThreadCastAccessor {
public:
  Thread series() const;
  Thread note() const;
  Thread timeline() const;
  Thread timeindex() const;
};
```

Calling the target that matches the source thread's own [`kind()`](kind.md) is always a
cheap identity — `Thread(...).cast().series()` on an already-`Series`-kind thread just
returns `*this` unchanged (subject to `options.series_dtype`, if you're also asking for a
different numeric dtype). Converting *between* kinds follows these rules:

- **→ `series()`**: `Timeline`/`TimeIndex` sources become their epoch-nanosecond integer
  ticks (as `Int64`, further converted to `options.series_dtype` if set). `Note` sources
  are parsed as numbers (`Float64` by default, or `Bool` if `series_dtype` is `Bool`,
  using permissive true/false-like text matching).
- **→ `note()`**: every other kind is formatted as text — `Timeline`/`TimeIndex` as ISO
  strings, `Series` via each scalar's own text representation.
- **→ `timeline()`**: `TimeIndex` sources are unwrapped directly. `Series` sources must be
  numeric and are interpreted either as epoch-nanosecond ticks or, when every valid value
  looks like an 8-digit `YYYYMMDD` integer, as calendar dates. `Note` sources are parsed as
  ISO-ish datetime text (or via `options.format`, if set).
- **→ `timeindex()`**: delegates to `timeline()` first, then wraps the result as an
  (unsorted-tolerant) `TimeIndex`.

```{important} ThreadCastOptions::invalid only actually takes effect for the Timeline target
```cpp
struct ThreadCastOptions {
  std::optional<pluss::codem::ScalarType> series_dtype{std::nullopt};
  std::string format{};
  ParseInvalid invalid{ParseInvalid::Raise};
};
```
`invalid` (`Raise` or `Null`) is meant to choose what happens when a `Note` value can't be
parsed. In the current implementation, that choice is only actually consulted by the
`Timeline` (and, transitively, `TimeIndex`) target: `Raise` (the default) throws on the
first unparseable value there, `Null` marks it invalid instead. For the **`Series`**
target, unparseable `Note` text is **always** silently marked `NULL` — `invalid` is never
consulted at all, regardless of which value you set it to. See Examples 3 and 4 for both
sides of this.
```

## Parameters

**options** (`const ThreadCastOptions&`, optional)
    `series_dtype` picks the concrete numeric dtype for a `series()` conversion (defaults
    to `Float64`, or effectively `Bool` when you explicitly request it). `format`, if
    non-empty, is a strptime-style pattern used to parse `Note` text for a `timeline()`
    (or `timeindex()`) conversion instead of the default ISO-ish parser. `invalid` chooses
    `Raise`-vs-`Null` behavior — see the important callout above for exactly which target
    actually honors it.

## Returns

`ThreadCastAccessor`
    A lightweight accessor bound to the current thread (and `options`, if given). Each of
    its four methods performs one conversion and returns a `Thread`.

## Examples

Example 1. Parse numeric text into a `Series`, using the default `Float64` dtype.

```cpp
using namespace pluss::table;

auto texts = Thread(pluss::make_note({"1.0", "2.0", "3.0"}));

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

Example 2. Request a specific `series_dtype` via `ThreadCastOptions`.

```cpp
ThreadCastOptions options;
options.series_dtype = pluss::codem::ScalarType::Int64;

auto out = Thread(pluss::make_note({"1", "2", "3"})).cast(options).series();
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
auto messy = Thread(pluss::make_note({"4.0", "oops", "6.0"}));

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
underlying datetime parser, not a `Thread`-specific wrapper.

```cpp
auto bad_dates = Thread(pluss::make_note({"2024-01-01T00:00:00Z", "not a date"}));
try {
  auto out = bad_dates.cast().timeline();
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}

ThreadCastOptions null_on_invalid;
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

Example 5. Cast a numeric thread to text with `note()`.

```cpp
auto amounts = Thread(pluss::make_series({1.5, 2.5}));

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
  happens in `.series()`/`.note()`/`.timeline()`/`.timeindex()`.
- `ThreadCastOptions::invalid` only actually affects the `Timeline`/`TimeIndex` targets —
  see the important callout above. `astype()`, which delegates to `cast().series()`
  internally, inherits this same "always null on unparseable text" behavior.
- Converting to a thread's own current kind is a cheap identity (subject to
  `series_dtype`, if a different numeric dtype is also requested).

## See Also

- [`Thread::astype`](astype.md): the single-argument shorthand for
  `cast(options).series()`.
- [`Thread::as_series`](as_series.md) / [`as_note`](as_note.md) /
  [`as_timeline`](as_timeline.md): view the current
  payload without converting it, throwing if the kind doesn't already match.
- [`DataFrame::cast`](../dataframe/cast.md): cast several frame features in one call.

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
