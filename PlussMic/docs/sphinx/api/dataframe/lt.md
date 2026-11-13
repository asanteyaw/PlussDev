<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9c03cada3d6b -->

# DataFrame::lt

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::lt(double other, bool strict = true) const
DataFrame DataFrame::lt(std::string_view other, bool strict = true) const
DataFrame DataFrame::lt(const DataFrame& other, bool strict = true) const
DataFrame DataFrame::lt(const DataFrame& other, const AlignOptions& options, bool strict = true) const
DataFrame DataFrame::lt(const Slate& other, Axis axis = Axis::Feature, bool strict = true) const
DataFrame DataFrame::lt(const Slate& other, Axis axis, const AlignOptions& options, bool strict = true) const
DataFrame DataFrame::lt(const Slate& other, const AlignOptions& options, Axis axis = Axis::Feature, bool strict = true) const
```

## What It Does

The ordering relation "less than", returning a boolean-valued frame the same shape as
the source. `lt()` shares the same scalar/string/`DataFrame`/`Slate` operand shapes as
[`eq()`](eq.md) — see `eq()`'s "What It Does" for the full explanation — plus one
extra parameter, `strict`, that the equality/inequality methods don't have.

```{important} le() is implemented in terms of lt()
[`le()`](le.md) has no independent implementation: `frame.le(other)` is literally
`frame.lt(other, /* strict = */ false)`. There is no separate "less than or equal"
code path — passing `strict = false` to `lt()` directly produces the exact same result
as calling `le()`.
```

## Parameters

**other**

The right-hand operand: `double`, `std::string_view` (`Note`-kind features only),
`const DataFrame&`, or `const Slate&` (broadcast per `axis`). See [`eq()`](eq.md) for
the full breakdown.

**strict** (`bool`, default `true`)

Selects which relation is actually evaluated:

- `true` (the default): strict "less than" (`<`) — a cell equal to `other` reports
  `false`.
- `false`: "less than or equal to" (`<=`) — a cell equal to `other` reports `true`.
  This is exactly what calling [`le()`](le.md) does internally.

**axis** (`Axis`, default `Axis::Feature`, Slate overloads only)

Same meaning as [`eq()`](eq.md)'s `axis`.

**options** (`AlignOptions`, optional)

Same meaning as [`eq()`](eq.md)'s `options`.

## Returns

`DataFrame`
    A new frame, same shape as the source, where every feature holds `true`/`false`
    according to whether that cell is less than (or, with `strict = false`, less than
    or equal to) `other`. A cell compared against a missing value on either side is
    itself missing (`NULL`), matching [`eq()`](eq.md).

## Examples

Example 1. Strict "less than" against a scalar threshold.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.lt(85.0);
std::cout << out;
```

### Output

```text
===================
     | quiz   final
-------------------
s1   | true   true 
s2   | false  false
s3   | true   true 
-------------------
[3 cs x 2 feat]
```

`84 < 85` is `true`, `90 < 85` is `false`, and `72 < 85` is `true` — an ordinary strict
comparison.

Example 2. Passing `strict = false` reproduces [`le()`](le.md) exactly.

```cpp
auto out_le_style = scores.lt(84.0, false);
std::cout << out_le_style;
```

### Output

```text
===================
     | quiz   final
-------------------
s1   | true   true 
s2   | false  false
s3   | true   true 
-------------------
[3 cs x 2 feat]
```

With `strict = false`, `84 < 84` (which would be `false` under the default) now
reports `true` for every case where the value equals `84` (`s1` and `s3` on both
features), since `84 <= 84` holds — `scores.le(84.0)` would produce this identical
result.

## Notes

- `le()` has no independent implementation — it is always `lt(other, false)` under the
  hood, for every overload shape.
- A cell compared against a missing value on either side is `NULL`, not `false`,
  matching [`eq()`](eq.md).
- Shares its full comparison-family overload shape with [`eq()`](eq.md), [`ne()`](ne.md),
  [`le()`](le.md), `gt()`, and `ge()`.

## See Also

- [`DataFrame::le`](le.md): the non-strict counterpart, implemented directly in terms
  of this method.
- [`DataFrame::eq`](eq.md): the full explanation of this comparison family's shared
  operand shapes.
- `DataFrame::gt` / `DataFrame::ge`: the mirror-image ordering relations.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lor.html">
    <span class="label">Previous</span>
    <span class="title">lor</span>
  </a>
  <a class="next-link" href="lxor.html">
    <span class="label">Next</span>
    <span class="title">lxor</span>
  </a>
</div>
