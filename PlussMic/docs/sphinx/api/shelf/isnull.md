<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 37cf2ec2178f -->

# Shelf::isnull

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::isnull() const
```

## What It Does

Returns a `Bool`-typed `Series` shelf, same length as `*this`, marking `true`
wherever the source shelf is `NULL` and `false` everywhere else. Works on every
payload kind — `Series`, `Note`, `Timeline`, `TimeIndex` — with no kind restriction,
unlike most of the payload-specific methods elsewhere in this API.

```{important} isnull() checks the validity mask, not the numeric value
For a `Series`-kind shelf, `isnull()` only looks at the shelf's validity mask — a
`NaN` value that's still marked *valid* is reported `false` by `isnull()`, not
`true`. Use [`isnan()`](isnan.md) to check for `NaN` specifically, or
[`invalid()`](invalid.md) to catch both `NULL` and non-finite values (`NaN`/`Inf`)
in one call. See Example 2.
```

For a `Timeline`/`TimeIndex`-kind shelf, `isnull()` also folds in the payload's own
internal validity signal (its own "not-a-time" sentinel), in addition to the
shelf's validity mask — so a timestamp that's intrinsically invalid is reported as
`NULL` here even without an explicit mask entry.

## Parameters

`None`

## Returns

`Shelf`
    A new `Bool`-typed `Series` shelf, same length as `*this`.

## Examples

Example 1. `isnull()` works the same way across every payload kind.

```cpp
using namespace pluss::table;

auto numbers = Shelf(pluss::make_series({1.0, 2.0, 3.0}),
    pluss::make_series(std::vector<bool>{true, false, true}));
auto out = numbers.isnull();
std::cout << out;
```

### Output

```text
0  false
1  true
2  false

ptype: Series
```

Example 2. `isnull()` is validity-mask-only — a `NaN` that's still marked valid is
`false`, not `true`. Compare with [`isnan()`](isnan.md).

```cpp
auto readings = Shelf(pluss::make_series({1.0, std::nan(""), 3.0}));
std::cout << readings.isnull() << "\n";
std::cout << readings.isnan();
```

### Output

```text
0  false
1  false
2  false

ptype: Series
0  false
1  true
2  false

ptype: Series
```

`readings` has no validity mask attached at all, so `isnull()` reports every
position as valid (`false`) — even the `NaN` at index `1`. `isnan()`, which checks
the actual floating-point value, correctly flags it.

## Notes

- `isnull` finds missing cells only. A `NaN` or an infinity is a **value**, so it is not flagged here: ask [`isnan`](isnan.md) or [`isinf`](isinf.md) for those, and [`invalid`](invalid.md) for all three at once. See [Missing, NaN and infinity](../../guide/missing_nan_and_inf.md).
- Works on every payload kind, with no `kind()` restriction.
- Checks the validity mask (plus, for `Timeline`/`TimeIndex`, the payload's own
  internal invalid-timestamp signal) — not the numeric value. See the important
  callout above.
- A shelf with no validity mask at all reports `false` (not null) at every
  position.

## See Also

- [`Shelf::notnull`](notnull.md): the logical inverse.
- [`Shelf::isnan`](isnan.md): checks the floating-point value itself, for
  `Series`-kind shelves.
- [`Shelf::invalid`](invalid.md): the union of `isnull()` and non-finite, for
  `Series`-kind shelves.
- [`Shelf::dropnull`](dropnull.md): remove `NULL` rows instead of just flagging
  them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isneginf.html">
    <span class="label">Previous</span>
    <span class="title">isneginf</span>
  </a>
  <a class="next-link" href="isposinf.html">
    <span class="label">Next</span>
    <span class="title">isposinf</span>
  </a>
</div>
