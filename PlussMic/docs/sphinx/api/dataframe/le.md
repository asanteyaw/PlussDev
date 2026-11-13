<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 703b2322c955 -->

# DataFrame::le

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::le(double other) const
DataFrame DataFrame::le(std::string_view other) const
DataFrame DataFrame::le(const DataFrame& other) const
DataFrame DataFrame::le(const DataFrame& other, const AlignOptions& options) const
DataFrame DataFrame::le(const Slate& other, Axis axis = Axis::Feature) const
DataFrame DataFrame::le(const Slate& other, Axis axis, const AlignOptions& options) const
DataFrame DataFrame::le(const Slate& other, const AlignOptions& options, Axis axis = Axis::Feature) const
```

## What It Does

The ordering relation "less than or equal to", returning a boolean-valued frame the
same shape as the source. `le()` shares the same scalar/string/`DataFrame`/`Slate`
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
`const DataFrame&`, or `const Slate&` (broadcast per `axis`). See [`eq()`](eq.md) for
the full breakdown.

**axis** (`Axis`, default `Axis::Feature`, Slate overloads only)

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

Example 2. Broadcast one text value against a `Note`-kind feature — lexicographic
comparison, same ordering `std::string` itself uses.

```cpp
auto names = DataFrame({
    {"name", pluss::make_note({"Ada", "Bola", "Ada"})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out_names = names.le("Ada");
std::cout << out_names;
```

### Output

```text
=============
      | name 
-------------
s1    | true 
s2    | false
s3    | true 
-------------
[3 cs x 1 feat]
```

`"Ada" <= "Ada"` holds (the boundary case), but `"Bola" <= "Ada"` does not —
`"Bola"` sorts after `"Ada"` lexicographically.

Example 3. Compare two frames cell by cell once they share the same structure.

```cpp
auto changed = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 83.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out_frames = scores.le(changed);
std::cout << out_frames;
```

### Output

```text
====================
      | quiz   final
--------------------
s1    | true   true 
s2    | true   true 
s3    | false  true 
--------------------
[3 cs x 2 feat]
```

Only `s3`'s `quiz` fails: `scores`'s `84` is not `<=` `changed`'s `83`. Every equal
cell (all of `final`, and `quiz` at `s1`/`s2`) reports `true`, since equality
satisfies `<=`.

Example 4. Compare against a `Slate` with one value per feature — `quiz` checked
against `84`, `final` checked against `88`.

```cpp
auto thresholds = Slate(pluss::make_series({84.0, 88.0}));
auto out_slate = scores.le(thresholds);
std::cout << out_slate;
```

### Output

```text
====================
      | quiz   final
--------------------
s1    | true   true 
s2    | false  true 
s3    | true   true 
--------------------
[3 cs x 2 feat]
```

`quiz <= 84` fails only at `s2` (`90`); `final <= 88` holds everywhere, since every
`final` value is at most `88`.

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
  <a class="next-link" href="length.html">
    <span class="label">Next</span>
    <span class="title">length</span>
  </a>
</div>
