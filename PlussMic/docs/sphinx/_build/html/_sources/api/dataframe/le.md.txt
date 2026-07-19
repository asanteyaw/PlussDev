<!-- pluss-hand-authored -->

# DataFrame::le

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::le(double other) const -> DataFrame
DataFrame::le(std::string_view other) const -> DataFrame
DataFrame::le(const DataFrame& other) const -> DataFrame
DataFrame::le(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::le(const Thread& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::le(const Thread& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::le(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The ordering relation "less than or equal to", returning a boolean-valued frame the
same shape as the source. `le()` shares the same scalar/string/`DataFrame`/`Thread`
operand shapes as [`eq()`](eq.md) — see `eq()`'s "What It Does" for the full
explanation.

```{important} le() has no independent implementation
Every `le()` overload is a thin wrapper: `frame.le(other, ...)` always calls straight
through to [`lt(other, ..., /* strict = */ false)`](lt.md). There is no `strict`
parameter on `le()` itself — if you need the strict "less than" relation on the same
call site, use [`lt()`](lt.md) directly instead.
```

## Parameters

**other**

The right-hand operand: `double`, `std::string_view` (`Note`-kind features only),
`const DataFrame&`, or `const Thread&` (broadcast per `axis`). See [`eq()`](eq.md) for
the full breakdown.

**axis** (`Axis`, default `Axis::Feature`, Thread overloads only)

Same meaning as [`eq()`](eq.md)'s `axis`.

**options** (`AlignOptions`, optional)

Same meaning as [`eq()`](eq.md)'s `options`.

## Returns

`DataFrame`
    A new frame, same shape as the source, where every feature holds `true`/`false`
    according to whether that cell is less than or equal to `other`. A cell compared
    against a missing value on either side is itself missing (`NULL`), matching
    [`eq()`](eq.md).

## Examples

Example 1. "Less than or equal to" against a scalar threshold.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.le(88.0);
std::cout << out;
```

### Output

```text
===================
     | quiz   final
-------------------
s1   | true   true 
s2   | false  true 
s3   | true   true 
-------------------
[3 cs x 2 feat]
```

`90 <= 88` is the only `false` here — every other cell in both features is at or below
`88`. Note that `final`'s `s2` value (`88`) reports `true` because the boundary value
itself counts as satisfying `<=`, unlike the strict [`lt()`](lt.md) relation.

## Notes

- `le()` always delegates to [`lt(other, false)`](lt.md); there is no separate code
  path and no `strict` parameter on `le()` itself.
- A cell compared against a missing value on either side is `NULL`, not `false`,
  matching [`eq()`](eq.md).
- Shares its full comparison-family overload shape with [`eq()`](eq.md), [`ne()`](ne.md),
  [`lt()`](lt.md), `gt()`, and `ge()`.

## See Also

- [`DataFrame::lt`](lt.md): the strict relation this method delegates to, and where the
  `strict` parameter actually lives.
- [`DataFrame::eq`](eq.md): the full explanation of this comparison family's shared
  operand shapes.
- `DataFrame::gt` / `DataFrame::ge`: the mirror-image ordering relations.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="land.html">
    <span class="label">Previous</span>
    <span class="title">land</span>
  </a>
  <a class="next-link" href="lengthen.html">
    <span class="label">Next</span>
    <span class="title">lengthen</span>
  </a>
</div>
