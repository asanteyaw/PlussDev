<!-- pluss-hand-authored -->

# DataFrame::ne

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::ne(double other) const -> DataFrame
DataFrame::ne(std::string_view other) const -> DataFrame
DataFrame::ne(const DataFrame& other) const -> DataFrame
DataFrame::ne(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::ne(const Thread& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::ne(const Thread& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::ne(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The negated counterpart of [`eq()`](eq.md): elementwise "not equal", returning a
boolean-valued frame the same shape as the source. `ne()` shares the exact same
overload shape as `eq()` — a scalar, a string, another `DataFrame`, or a `Thread`
broadcast by `axis` — see `eq()`'s "What It Does" for the full explanation of the
operand shapes and the `axis`/`options` parameters.

## Parameters

**other**

The right-hand operand: `double` (numeric scalar), `std::string_view` (text value,
`Note`-kind features only), `const DataFrame&` (aligned frame-to-frame), or `const
Thread&` (broadcast per `axis`). See [`eq()`](eq.md) for the full breakdown.

**axis** (`Axis`, default `Axis::Feature`, Thread overloads only)

Same meaning as [`eq()`](eq.md)'s `axis`.

**options** (`AlignOptions`, optional)

Same meaning as [`eq()`](eq.md)'s `options`.

## Returns

`DataFrame`
    A new frame, same shape as the source, where every feature holds `true`/`false`
    according to whether that cell differs from `other`. A cell compared against a
    missing value on either side is itself missing (`NULL`), matching [`eq()`](eq.md).

## Examples

Example 1. Broadcast one numeric scalar across every numeric feature.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.ne(84.0);
std::cout << out;
```

### Output

```text
===================
     | quiz   final
-------------------
s1   | false  false
s2   | true   true 
s3   | false  true 
-------------------
[3 cs x 2 feat]
```

Every cell here is the exact opposite of what [`scores.eq(84.0)`](eq.md) would report
— `ne()` and `eq()` always partition the same cells into complementary `true`/`false`
sets, except where either side is missing (`NULL` stays `NULL` under both).

Example 2. Broadcast one text value against a `Note`-kind feature.

```cpp
auto names = DataFrame({
    {"name", pluss::make_note({"Ada", "Bola", "Ada"})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out_names = names.ne("Bola");
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

Only `s2`'s `"Bola"` matches the compared value, so it's the sole `false` — everything
else differs and reports `true`.

Example 3. Compare two frames cell by cell — only `s3`'s `quiz` value actually
differs.

```cpp
auto changed = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 83.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out_frames = scores.ne(changed);
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

Exactly one `true` cell — `s3`'s `quiz` (`84` vs. `83`) — the complement of what
[`scores.eq(changed)`](eq.md) reports for the same two frames.

Example 4. Compare against a `Thread` with one value per feature — `quiz` checked
against `84`, `final` checked against `88`.

```cpp
auto thresholds = Thread(pluss::make_series({84.0, 88.0}));
auto out_thread = scores.ne(thresholds);
std::cout << out_thread;
```

### Output

```text
====================
      | quiz   final
--------------------
s1    | false  true 
s2    | true   false
s3    | false  true 
--------------------
[3 cs x 2 feat]
```

Each feature is checked against its own threshold, independently per column, exactly
as [`eq()`](eq.md)'s matching example does — every cell here is that example's logical
complement.

## Notes

- `ne()` is the logical negation of [`eq()`](eq.md), cell for cell, with the one
  exception of missing-value cells, which stay `NULL` under both rather than flipping
  to the opposite boolean.
- Shares its full overload shape and alignment semantics with [`eq()`](eq.md),
  [`lt()`](lt.md), [`le()`](le.md), `gt()`, and `ge()`.

## See Also

- [`DataFrame::eq`](eq.md): the equality relation, and the full explanation of this
  comparison family's shared operand shapes.
- [`DataFrame::lt`](lt.md) / [`DataFrame::le`](le.md): the ordering relations.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="nfeatures.html">
    <span class="label">Next</span>
    <span class="title">nfeatures</span>
  </a>
</div>
