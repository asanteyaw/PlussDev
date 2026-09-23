<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9625712967b3 -->

# DataFrame::compare

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::compare(double other) const -> DataFrame
DataFrame::compare(const DataFrame& other) const -> DataFrame
DataFrame::compare(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::compare(const Thread& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::compare(const Thread& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::compare(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

A plain alias for [`ne()`](ne.md): every `compare()` overload forwards straight
through to the matching `ne()` overload with the same arguments, with no independent
behavior of its own. It exists as an alternate, more generic-sounding name for the
same not-equal comparison — some call sites read more naturally as "compare this
frame against a baseline" than "check where it's not-equal to a baseline," even though
the two produce byte-for-byte identical results.

```{important} compare() is ne(), not a general comparator
Despite the generic name, `compare()` does not let you choose which relation to
evaluate — it is hardwired to not-equal. For any other relation, call
[`eq()`](eq.md), [`lt()`](lt.md), [`le()`](le.md), `gt()`, or `ge()` directly.
```

## Parameters

**other**

The right-hand operand: `double` (numeric scalar), `const DataFrame&` (aligned
frame-to-frame), or `const Thread&` (broadcast per `axis`). Identical in every respect
to [`ne()`](ne.md)'s `other` — there is no `std::string_view` overload for
`compare()`, unlike `ne()`, `eq()`, `lt()`, `le()`, `gt()`, and `ge()`.

**axis** (`Axis`, default `Axis::Feature`, Thread overloads only)

Same meaning as [`eq()`](eq.md)'s `axis`.

**options** (`AlignOptions`, optional)

Same meaning as [`eq()`](eq.md)'s `options`.

## Returns

`DataFrame`
    A new frame, same shape as the source, where every feature holds `true`/`false`
    according to whether that cell differs from `other` — identical to what
    [`ne(other)`](ne.md) would return for the same arguments.

## Examples

Example 1. `compare()` and `ne()` produce byte-for-byte identical results.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.compare(84.0);
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

```cpp
auto out_ne = scores.ne(84.0);
std::cout << out_ne;
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

Same call arguments, same output — `compare(84.0)` is `ne(84.0)` under a different
name, with nothing distinguishing the two at the result level.

Example 2. The `Thread` overload's `axis` parameter, left at its default
`Axis::Feature`: one threshold value per feature, broadcast down every case in
that column.

```cpp
auto per_feature = Thread(pluss::make_series({84.0, 88.0}));
auto out_feature = scores.compare(per_feature);
std::cout << out_feature;
```

### Output

```text
===================
     | quiz   final
-------------------
s1   | false  true 
s2   | true   false
s3   | false  true 
-------------------
[3 cs x 2 feat]
```

With `axis` defaulted to `Axis::Feature`, `per_feature`'s two values pair up with
`quiz` and `final` respectively: every row's `quiz` is checked against `84`
(`false, true, false`), and every row's `final` is checked against `88` (`true,
false, true`), independent of one another.

Example 3. The same `Thread` overload with `axis = Axis::Case`: one threshold
value per case instead, broadcast across every feature in that row.

```cpp
auto per_case = Thread(pluss::make_series({84.0, 90.0, 72.0}));
auto out_case = scores.compare(per_case, Axis::Case);
std::cout << out_case;
```

### Output

```text
===================
     | quiz   final
-------------------
s1   | false  false
s2   | false  true 
s3   | true   false
-------------------
[3 cs x 2 feat]
```

Now `per_case`'s three values pair up with `s1`, `s2`, and `s3` instead: `s1`'s
`quiz` and `final` are both checked against `84` (`false, false` — both cells
equal `84`), `s2`'s row is checked against `90` (`false, true`), and `s3`'s row
against `72` (`true, false`) — the opposite broadcast direction from Example 2,
using the exact same two-argument overload.

## Notes

- There is no `std::string_view` overload here, unlike the rest of the comparison
  family — text comparison must go through [`ne()`](ne.md) (or [`eq()`](eq.md))
  directly.
- Missing-value propagation is identical to [`ne()`](ne.md): a cell compared against a
  missing value on either side is itself missing (`NULL`), not `false`.

## See Also

- [`DataFrame::ne`](ne.md): the method `compare()` forwards to, and the canonical name
  to reach for.
- [`DataFrame::eq`](eq.md): the full explanation of this comparison family's shared
  operand shapes.
- `Thread::compare`: the lower-level typed comparison surface this ultimately reduces
  to on each column.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="combine.html">
    <span class="label">Previous</span>
    <span class="title">combine</span>
  </a>
  <a class="next-link" href="conform.html">
    <span class="label">Next</span>
    <span class="title">conform</span>
  </a>
</div>
