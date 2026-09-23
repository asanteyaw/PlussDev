<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 11ff6a27594a -->

# Thread::astype

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::astype(pluss::codem::ScalarType dtype) const -> Thread
```

## What It Does

Converts a thread to a different concrete scalar dtype, staying on the `Thread` surface
throughout — a thin, single-argument shorthand for
[`cast(options).series()`](cast.md) with `options.series_dtype = dtype`. Works from any
payload kind: a `Series`-kind thread simply reinterprets its values at the new dtype;
`Note`-, `Timeline`-, and `TimeIndex`-kind threads first parse or convert to numeric
before landing at the requested dtype, following the same rules described on
[`cast()`](cast.md)'s page (including that page's important callout about
`ThreadCastOptions::invalid` only actually taking effect for the `Timeline` target — an
`astype()` call on unparseable `Note` text never throws, it silently produces `NULL`).

## Parameters

**dtype** (`pluss::codem::ScalarType`)
    The target scalar dtype — for example `pluss::codem::ScalarType::Bool`,
    `Int64`, or `Float64`. Choose the smallest dtype that still supports whatever
    downstream operation you plan to run.

## Returns

`Thread`
    A new `Series`-kind thread holding the converted values. The row validity mask is
    preserved (or, for `Note`-kind sources, extended to mark unparseable entries `NULL`).

## Examples

Example 1. Convert a `0`/`1` numeric thread into a boolean thread.

```cpp
using namespace pluss::table;

auto flags = Thread(pluss::make_series({1.0, 0.0, 1.0}));

auto out = flags.astype(pluss::codem::ScalarType::Bool);
std::cout << out;
```

### Output

```text
0  true
1  false
2  true
ptype: Series
```

Example 2. `astype()` also works starting from a `Note`-kind thread — it parses the text
first, exactly like [`cast(...).series()`](cast.md) would.

```cpp
auto texts = Thread(pluss::make_note({"1", "2", "3"}));

auto out = texts.astype(pluss::codem::ScalarType::Int64);
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

Example 3. Unparseable `Note` text becomes `NULL`, not a throw — `astype()` has no
`ThreadCastOptions` to configure that behavior, so it always takes the "silently null"
path.

```cpp
auto messy = Thread(pluss::make_note({"4", "oops", "6"}));

auto out = messy.astype(pluss::codem::ScalarType::Int64);
std::cout << out;
```

### Output

```text
0  4
1  NULL
2  6
ptype: Series
```

## Notes

- A thin shorthand for [`cast(options).series()`](cast.md) with `options.series_dtype`
  set — see that page for the full conversion rules per source payload kind.
- Works from any payload kind, not just `Series` — `Note`/`Timeline`/`TimeIndex` sources
  are converted to numeric first.
- Takes no `ThreadCastOptions`, so there's no way to request "throw on unparseable text"
  through this method — use [`cast()`](cast.md) directly if you need that.

## See Also

- [`Thread::cast`](cast.md): the full, options-configurable casting accessor this method
  is a shorthand for.
- [`Thread::as_series`](as_series.md): drop to the raw `Series` payload for lower-level
  numeric control instead of converting dtype.
- [`DataFrame::astype`](../dataframe/astype.md): the same conversion, applied to several
  frame features at once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="as_timeline.html">
    <span class="label">Previous</span>
    <span class="title">as_timeline</span>
  </a>
  <a class="next-link" href="between.html">
    <span class="label">Next</span>
    <span class="title">between</span>
  </a>
</div>
