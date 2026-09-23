<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 86d8621b0095 -->

# DataFrame::eq

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::eq(double other) const -> DataFrame
DataFrame::eq(std::string_view other) const -> DataFrame
DataFrame::eq(const DataFrame& other) const -> DataFrame
DataFrame::eq(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::eq(const Thread& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::eq(const Thread& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::eq(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The named-method form of elementwise equality, returning a boolean-valued frame the
same shape as the source. `eq()` is one of six comparison methods
(`eq`, [`ne`](ne.md), [`lt`](lt.md), [`le`](le.md), `gt`, `ge`) that share the same
overload set — a scalar, a string, another `DataFrame`, or a `Thread` broadcast by
`axis` — mirroring the arithmetic family's shape (see [`add()`](add.md)), except every
comparison method returns a `Series<bool>` per feature instead of a numeric result.

Reach for the named method over the `==` operator when you want to chain it inline in
a pipeline, or when comparing against another frame with non-default alignment rules.

## Parameters

**other**

The right-hand operand, in one of three shapes:

- `double`: one numeric scalar, compared against every numeric feature.
- `std::string_view`: one text value, compared against every `Note`-kind feature.
  Comparing a non-`Note` feature against a string is not a supported combination.
- `const DataFrame&`: another frame, compared feature-by-feature and case-by-case
  after the two frames are aligned.
- `const Thread&`: one column of values, broadcast according to `axis`.

**axis** (`Axis`, default `Axis::Feature`, Thread overloads only)

Controls how a `Thread` right-hand operand is broadcast — identical meaning to
[`add()`](add.md)'s `axis`: `Axis::Feature` (the default) supplies one value per
feature; `Axis::Case` supplies one value per case.

**options** (`AlignOptions`, optional)

Controls how the `DataFrame` or `Thread` right-hand operand is aligned to the current
frame before the comparison runs. Same meaning as [`add()`](add.md)'s `options`.

## Returns

`DataFrame`
    A new frame, same shape as the source, where every feature holds `true`/`false`
    according to whether that cell equals `other`. A cell compared against a missing
    value on either side is itself missing (`NULL`) rather than `false`.

## Examples

Example 1. Broadcast one numeric scalar across every numeric feature.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

std::cout << scores;
```

### Output (before)

```text
==================
     | quiz  final
------------------
s1   | 84    84   
s2   | 90    88   
s3   | 84    72   
------------------
[3 cs x 2 feat]
```

```cpp
auto out = scores.eq(84.0);
std::cout << out;
```

### Output (after)

```text
===================
     | quiz   final
-------------------
s1   | true   true 
s2   | false  false
s3   | true   false
-------------------
[3 cs x 2 feat]
```

Example 2. Broadcast one text value against a `Note`-kind feature.

```cpp
auto names = DataFrame({
    {"name", pluss::make_note({"Ada", "Bola", "Ada"})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out_names = names.eq("Ada");
std::cout << out_names;
```

### Output

```text
============
     | name 
------------
s1   | true 
s2   | false
s3   | true 
------------
[3 cs x 1 feat]
```

Example 3. Compare two frames cell by cell once they share the same structure.

```cpp
auto changed = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 83.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out_frames = scores.eq(changed);
std::cout << out_frames;
```

### Output

```text
===================
     | quiz   final
-------------------
s1   | true   true 
s2   | true   true 
s3   | false  true 
-------------------
[3 cs x 2 feat]
```

Only `s3`'s `quiz` value differs between `scores` (`84`) and `changed` (`83`) — every
other cell still compares equal.

Example 4. Compare against a `Thread` with one value per feature — `quiz` checked
against `84`, `final` checked against `88`.

```cpp
auto thresholds = Thread(pluss::make_series({84.0, 88.0}));
auto out_thread = scores.eq(thresholds);
std::cout << out_thread;
```

### Output

```text
===================
     | quiz   final
-------------------
s1   | true   false
s2   | false  true 
s3   | true   false
-------------------
[3 cs x 2 feat]
```

Each feature is checked against its own threshold: `quiz == 84` gives `true, false,
true`, while `final == 88` gives `false, true, false` — independent per-column
comparisons in a single call.

## Notes

- A cell where either operand is missing (`NULL`) produces `NULL` in the result, not
  `false` — a missing value is never considered equal or unequal to anything, including
  another missing value.
- String comparison only applies to `Note`-kind features; attempting it against a
  numeric feature is not a supported combination.
- Shares its full overload shape and alignment semantics with [`ne()`](ne.md),
  [`lt()`](lt.md), [`le()`](le.md), `gt()`, and `ge()` — see those pages for the
  distinct relations, and [`add()`](add.md) for the fuller explanation of the shared
  `axis`/`options` parameters.

## See Also

- [`DataFrame::ne`](ne.md): the negated relation.
- [`DataFrame::equals`](equals.md): a single `bool` verdict for whole-frame identity,
  instead of a per-cell mask.
- [`DataFrame::add`](add.md): the arithmetic family this comparison family's overload
  shape mirrors.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="encode.html">
    <span class="label">Previous</span>
    <span class="title">encode</span>
  </a>
  <a class="next-link" href="equals.html">
    <span class="label">Next</span>
    <span class="title">equals</span>
  </a>
</div>
