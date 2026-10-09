<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 740e11e71946 -->

# Slate::invalid

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::invalid() const -> Slate
```

## What It Does

Returns a `Bool`-typed `Series` slate, same length as `*this`, marking `true`
wherever a cell should be treated as unusable data. The exact meaning depends on
payload kind:

- **`Series`-kind**: `true` for `NULL`, `NaN`, **or** `±Inf` — the union of
  [`isnull()`](isnull.md) and the logical negation of [`isfinite()`](isfinite.md).
  This is a strictly broader check than `isnull()` alone.
- **Every other kind** (`Note`, `Timeline`, `TimeIndex`): `invalid()` is exactly
  [`isnull()`](isnull.md) — there's no separate "not finite" concept for text or
  temporal payloads, so the two checks coincide.

```{important} invalid() is not isfinite()'s exact opposite for masked cells
For `Series`-kind slates, `invalid()` and [`isfinite()`](isfinite.md) are close to
opposites but computed slightly differently under the hood — `invalid()` is built as
"not finite, or explicitly null," which lands on the same result as `!isfinite()`
for every practical case, but the two are documented separately here because they're
implemented via genuinely different code paths, not simply negated from one another.
```

## Parameters

`None`

## Returns

`Slate`
    A new `Bool`-typed `Series` slate, same length as `*this`.

## Examples

Example 1. On a `Series`-kind slate, `invalid()` catches `NULL`, `NaN`, and `Inf`
alike.

```cpp
using namespace pluss::table;

auto readings = Slate(
    pluss::make_series({1.0, std::nan(""), std::numeric_limits<double>::infinity(), 4.0}),
    pluss::make_series(std::vector<bool>{true, true, true, false}));
std::cout << readings.invalid();
```

### Output

```text
0  false
1  true
2  true
3  true

ptype: Series
```

Every one of positions `1` through `3` is flagged `true`: `NaN`, `Inf`, and
masked-`NULL` are all "invalid" together — a strictly broader net than
[`isnull()`](isnull.md) alone would catch (which would only flag position `3`).

Example 2. On a `Note`-kind slate, `invalid()` is exactly `isnull()`.

```cpp
auto names = Slate(pluss::make_note({"Ada", "Bola", "Chen"}),
    pluss::make_series(std::vector<bool>{true, false, true}));
std::cout << names.invalid() << "\n";
std::cout << names.isnull();
```

### Output

```text
0  false
1  true
2  false

ptype: Series
0  false
1  true
2  false

ptype: Series
```

## Notes

- For `Series`-kind slates, `invalid()` is strictly broader than
  [`isnull()`](isnull.md) — it also catches `NaN` and `±Inf`.
- For every other kind, `invalid()` and `isnull()` produce identical results.
- Always returns a `Bool`-typed `Series` slate, regardless of the source slate's
  own kind.

## See Also

- [`Slate::isnull`](isnull.md): the narrower, validity-mask-only check this method
  extends for `Series`-kind slates.
- [`Slate::isfinite`](isfinite.md): the near-opposite numeric check.
- [`Slate::dropnull`](dropnull.md): remove rows rather than just flagging them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="interp.html">
    <span class="label">Previous</span>
    <span class="title">interp</span>
  </a>
  <a class="next-link" href="isfinite.html">
    <span class="label">Next</span>
    <span class="title">isfinite</span>
  </a>
</div>
