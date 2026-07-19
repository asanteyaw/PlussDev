<!-- pluss-hand-authored -->

# DataFrame::ge

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::ge(double other) const -> DataFrame
DataFrame::ge(std::string_view other) const -> DataFrame
DataFrame::ge(const DataFrame& other) const -> DataFrame
DataFrame::ge(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::ge(const Thread& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::ge(const Thread& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::ge(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The ordering relation "greater than or equal to", returning a boolean-valued frame the
same shape as the source. `ge()` shares the same scalar/string/`DataFrame`/`Thread`
operand shapes as [`eq()`](eq.md) — see `eq()`'s "What It Does" for the full
explanation.

```{important} ge() has no independent implementation
Every `ge()` overload is a thin wrapper: `frame.ge(other, ...)` always calls straight
through to [`gt(other, ..., /* strict = */ false)`](gt.md). There is no `strict`
parameter on `ge()` itself — if you need the strict "greater than" relation on the
same call site, use [`gt()`](gt.md) directly instead.
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
    according to whether that cell is greater than or equal to `other`. A cell compared
    against a missing value on either side is itself missing (`NULL`), matching
    [`eq()`](eq.md).

## Examples

Example 1. "Greater than or equal to" against a scalar threshold.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.ge(84.0);
std::cout << out;
```

### Output

```text
==================
     | quiz  final
------------------
s1   | true  true 
s2   | true  true 
s3   | true  false
------------------
[3 cs x 2 feat]
```

Every `quiz` value is at least `84`, so that feature is all `true`. `final` fails only
at `s3`, where `72 < 84` — the one cell that does not clear the threshold, strict or
not.

## Notes

- `ge()` always delegates to [`gt(other, false)`](gt.md); there is no separate code
  path and no `strict` parameter on `ge()` itself.
- A cell compared against a missing value on either side is `NULL`, not `false`,
  matching [`eq()`](eq.md).
- Shares its full comparison-family overload shape with [`eq()`](eq.md), [`ne()`](ne.md),
  [`lt()`](lt.md), [`le()`](le.md), and [`gt()`](gt.md).

## See Also

- [`DataFrame::gt`](gt.md): the strict relation this method delegates to, and where the
  `strict` parameter actually lives.
- [`DataFrame::le`](le.md): the mirror-image ordering relation.
- [`DataFrame::eq`](eq.md): the full explanation of this comparison family's shared
  operand shapes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="get.html">
    <span class="label">Next</span>
    <span class="title">get</span>
  </a>
</div>
