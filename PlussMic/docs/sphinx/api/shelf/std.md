<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6c1af90dbf47 -->

# Shelf::std

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
ShelfValue Shelf::std(bool unbiased = true) const
ShelfValue Shelf::std(const StatisticOptions& options) const
```

## What It Does

Returns the standard deviation of every observed (non-missing) value in a
numeric `Series` shelf — the square root of [`var()`](var.md)'s result, with
the exact same overload split and failure-mode difference:

- **`std(unbiased)`**: `unbiased = true` (the default) computes the sample
  standard deviation (`n - 1` divisor underneath); `unbiased = false` computes
  the population standard deviation (`n` divisor). An empty shelf, or one
  where every entry is missing, **throws**.
- **`std(options)`**: takes a `StatisticOptions{ddof, min_count}`. Instead of
  throwing on an insufficient sample, this overload returns a result marked
  *invalid* (see the important caveat below) when the observed count is below
  `min_count`.

See [`Shelf::sum`'s note on printing a `ShelfValue`](sum.md) — every example
below uses `.to_dbl()` for a clean number, and the note there also covers the
"invalid `ShelfValue` still prints a number" trap that Example 4 below runs
into.

## Parameters

**unbiased** (`bool`, default `true`)

`true`: sample standard deviation (`n - 1` divisor underneath). `false`:
population standard deviation (`n` divisor underneath).

**options** (`StatisticOptions`)

`ddof` (default `1`): the divisor underneath is `n - ddof`. `min_count`
(default `1`): the smallest observed count still considered valid; below
it, the result is marked invalid rather than thrown.

## Returns

`ShelfValue`
    A scalar `Float64` (or `Float32` off-CPU) holding the standard deviation.

## Examples

Example 1. Sample standard deviation (the default).

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({2.0, 4.0, 6.0, 8.0}));
auto out = values.std();
std::cout << out.to_dbl();
```

### Output

```text
2.58199
```

Example 2. Population standard deviation.

```cpp
auto out2 = values.std(false);
std::cout << out2.to_dbl();
```

### Output

```text
2.23607
```

Example 3. The `unbiased`-form throws on an empty (all-missing) selection.

```cpp
auto all_missing = Shelf(
    pluss::make_series({1.0, 2.0}),
    pluss::make_series(std::vector<bool>{false, false})
);
try {
  auto bad = all_missing.std();
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::std on an empty (all-null) selection
```

Example 4. The `StatisticOptions`-form returns an invalid result below
`min_count`, rather than throwing — check the scalar validity mask, not the
number itself.

```cpp
auto small = Shelf(pluss::make_series({10.0, 20.0}));
auto out4 = small.std(StatisticOptions{.ddof = 1, .min_count = 5});
std::cout << out4.shelf().notnull().as_series().host<bool>()[0];
```

### Output

```text
0
```

## Notes

- Missing entries are excluded before computing, in both overloads.
- The `unbiased`-form throws on an empty/all-missing selection; the
  `options`-form returns an invalid result below `min_count` instead of
  throwing.
- `ddof = 1` (the `options`-form's default) matches `unbiased = true`; `ddof =
  0` matches `unbiased = false`.
- Restricted to `Series` payloads; anything else throws `"Shelf::std is only
  valid for Series payload"`.

## See Also

- [`Shelf::var`](var.md): the variance this method takes the square root of.
- [`Shelf::se`](se.md): the standard error of the mean, a related but
  distinct spread measure.
- [`Shelf::describe`](describe.md): reports `std` as part of a wider numeric
  summary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="starts.html">
    <span class="label">Previous</span>
    <span class="title">starts</span>
  </a>
  <a class="next-link" href="strip.html">
    <span class="label">Next</span>
    <span class="title">strip</span>
  </a>
</div>
