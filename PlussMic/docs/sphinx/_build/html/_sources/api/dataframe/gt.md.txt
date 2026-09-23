<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 985827ed4fbb -->

# DataFrame::gt

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::gt(double other, bool strict = true) const -> DataFrame
DataFrame::gt(std::string_view other, bool strict = true) const -> DataFrame
DataFrame::gt(const DataFrame& other, bool strict = true) const -> DataFrame
DataFrame::gt(const DataFrame& other, const AlignOptions& options, bool strict = true) const -> DataFrame
DataFrame::gt(const Thread& other, Axis axis = Axis::Feature, bool strict = true) const -> DataFrame
DataFrame::gt(const Thread& other, Axis axis, const AlignOptions& options, bool strict = true) const -> DataFrame
DataFrame::gt(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature, bool strict = true) const -> DataFrame
```

## What It Does

The ordering relation "greater than", returning a boolean-valued frame the same shape
as the source. `gt()` is the mirror image of [`lt()`](lt.md): same operand shapes, same
`strict` parameter, same comparison-family alignment rules — see [`eq()`](eq.md)'s
"What It Does" for the full explanation of the shared scalar/string/`DataFrame`/`Thread`
operand shapes.

```{important} ge() is implemented in terms of gt()
Just like [`le()`](le.md) is `lt(other, false)` under the hood, [`ge()`](ge.md) has no
independent implementation: `frame.ge(other)` is literally `frame.gt(other, /* strict =
*/ false)`.
```

## Parameters

**other**

The right-hand operand: `double`, `std::string_view` (`Note`-kind features only),
`const DataFrame&`, or `const Thread&` (broadcast per `axis`). See [`eq()`](eq.md) for
the full breakdown.

**strict** (`bool`, default `true`)

- `true` (the default): strict "greater than" (`>`) — a cell equal to `other` reports
  `false`.
- `false`: "greater than or equal to" (`>=`) — a cell equal to `other` reports `true`.
  This is exactly what calling [`ge()`](ge.md) does internally.

**axis** (`Axis`, default `Axis::Feature`, Thread overloads only)

Same meaning as [`eq()`](eq.md)'s `axis`.

**options** (`AlignOptions`, optional)

Same meaning as [`eq()`](eq.md)'s `options`.

## Returns

`DataFrame`
    A new frame, same shape as the source, where every feature holds `true`/`false`
    according to whether that cell is greater than (or, with `strict = false`, greater
    than or equal to) `other`. A cell compared against a missing value on either side
    is itself missing (`NULL`), matching [`eq()`](eq.md).

## Examples

Example 1. Strict "greater than" against a scalar threshold.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.gt(85.0);
std::cout << out;
```

### Output

```text
===================
     | quiz   final
-------------------
s1   | false  false
s2   | true   true 
s3   | false  false
-------------------
[3 cs x 2 feat]
```

`s2` is the only case that clears the `85` threshold on both features: `90 > 85` and
`88 > 85` are both `true`, while every `84` and the `72` fall at or below it.

Example 2. Passing `strict = false` reproduces [`ge()`](ge.md) exactly.

```cpp
auto out_ge_style = scores.gt(84.0, false);
std::cout << out_ge_style;
```

### Output

```text
===================
      | quiz  final
-------------------
s1    | true  true 
s2    | true  true 
s3    | true  false
-------------------
[3 cs x 2 feat]
```

With `strict = false`, `84 > 84` (which would be `false` under the default) now
reports `true` for every case where the value equals `84`, since `84 >= 84` holds —
`scores.ge(84.0)` would produce this identical result.

Example 3. Broadcast one text value against a `Note`-kind feature — lexicographic
comparison, same ordering `std::string` itself uses.

```cpp
auto names = DataFrame({
    {"name", pluss::make_note({"Ada", "Bola", "Ada"})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out_names = names.gt("Ada");
std::cout << out_names;
```

### Output

```text
=============
      | name 
-------------
s1    | false
s2    | true 
s3    | false
-------------
[3 cs x 1 feat]
```

Only `"Bola"` sorts strictly after `"Ada"` — the two `"Ada"` cases fail the strict
relation against themselves.

Example 4. Compare two frames cell by cell once they share the same structure.

```cpp
auto changed = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 83.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out_frames = scores.gt(changed);
std::cout << out_frames;
```

### Output

```text
====================
      | quiz   final
--------------------
s1    | false  false
s2    | false  false
s3    | true   false
--------------------
[3 cs x 2 feat]
```

Exactly one `true` cell — `s3`'s `quiz` (`84 > 83`) — since every other cell in
`changed` is *equal* to `scores`, not strictly less; compare this against
[`scores.ge(changed)`](ge.md) on the identical two frames, where all of those equal
cells report `true` instead.

Example 5. Compare against a `Thread` with one value per feature — `quiz` checked
against `84`, `final` checked against `88`.

```cpp
auto thresholds = Thread(pluss::make_series({84.0, 88.0}));
auto out_thread = scores.gt(thresholds);
std::cout << out_thread;
```

### Output

```text
====================
      | quiz   final
--------------------
s1    | false  false
s2    | true   false
s3    | false  false
--------------------
[3 cs x 2 feat]
```

`quiz > 84` holds only at `s2` (`90`); `final > 88` never holds, since no `final`
value exceeds `88`.

## Notes

- `ge()` has no independent implementation — it is always `gt(other, false)` under the
  hood, mirroring how [`le()`](le.md) delegates to [`lt()`](lt.md).
- A cell compared against a missing value on either side is `NULL`, not `false`,
  matching [`eq()`](eq.md).
- Shares its full comparison-family overload shape with [`eq()`](eq.md), [`ne()`](ne.md),
  [`lt()`](lt.md), [`le()`](le.md), and [`ge()`](ge.md).

## See Also

- [`DataFrame::ge`](ge.md): the non-strict counterpart, implemented directly in terms
  of this method.
- [`DataFrame::lt`](lt.md) / [`DataFrame::le`](le.md): the mirror-image ordering
  relations.
- [`DataFrame::eq`](eq.md): the full explanation of this comparison family's shared
  operand shapes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="group.html">
    <span class="label">Previous</span>
    <span class="title">group</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
