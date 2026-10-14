<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0aa9dbe3a73a -->

# DataFrame::factor

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::factor(std::string_view feature, bool ordered = false, bool sort_levels = false) const -> CategoryEncoding
```

## What It Does

The lower-level label-encoding primitive: converts one feature's distinct values into
integer codes, returning both the codes *and* the level list that maps each code back
to its original value — a `CategoryEncoding{codes, levels, ordered}` struct rather than
a modified frame. [`encode()`](encode.md)'s `Ordinal` mode is a thin convenience
wrapper around exactly this call, discarding `levels` and splicing `codes` back into
the frame in place of `feature`. Reach for `factor()` directly (instead of
`encode(feature, EncodeMode::Ordinal)`) whenever you need the level list too — to
label a chart axis, to build a lookup table, or to apply the same code assignment to a
second frame later.

## Parameters

**feature** (`std::string_view`)

The feature to factor. Works on any `Slate` kind — "distinct value" is determined by
the same string-key comparison used throughout the categorical-encoding surface
(`unify_key_parts`), not restricted to `Note` text.

**ordered** (`bool`, default `false`)

Carried straight through into the returned `CategoryEncoding::ordered` flag. `factor()`
itself does not change its own behavior based on this flag — it exists so downstream
code consuming the `CategoryEncoding` can know whether the level order is meaningful
(an ordinal scale) or arbitrary (a plain nominal category), a distinction `factor()`
otherwise has no way to communicate.

**sort_levels** (`bool`, default `false`)

- `false` (the default): distinct values are ordered by first appearance in `feature`.
- `true`: distinct values are sorted lexicographically before codes are assigned.

This determines which value gets code `0`, `1`, `2`, and so on — and, since `levels[i]`
and code `i` always correspond, it also determines the order of the returned `levels`
slate.

## Returns

`CategoryEncoding`
    A plain struct with three fields: `codes` (`Slate`, `Int64`, one entry per case,
    `NULL` wherever `feature` itself was missing), `levels` (`Slate`, the distinct
    values in code order — code `i` corresponds to `levels[i]`), and `ordered` (`bool`,
    the value passed in). `CategoryEncoding` also exposes a `.decode()` method that
    reconstructs a `Slate` of original values from `codes` and `levels`.

## Examples

Example 1. Factor a text feature with the default first-seen ordering, including a
missing value.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",   Slate(pluss::make_series({1, 2, 3, 4, 5}))},
    {"city", Slate(pluss::make_note({"Tema", "Accra", "Tema", "Kumasi", ""}),
                    pluss::make_series({true, true, true, true, false}, pluss::ScalarType::Bool))},
});
std::cout << orders;
```

### Output

```text
===================
     | id    city  
-------------------
0    | 1     Tema  
1    | 2     Accra 
2    | 3     Tema  
3    | 4     Kumasi
4    | 5     NULL  
-------------------
[5 cs x 2 feat]
```

```cpp
auto factored = orders.factor("city");
std::cout << factored.codes;
std::cout << factored.levels;
```

### Output

```text
0  0
1  1
2  0
3  2
4  NULL
ptype: Series
```

```text
0  Tema
1  Accra
2  Kumasi
ptype: Note
```

`Tema` is the first distinct value encountered, so it gets code `0`; `Accra` (first
seen at case `1`) gets `1`; `Kumasi` (first seen at case `3`) gets `2`. Case `4`'s
missing city stays `NULL` in `codes` rather than becoming a fourth, spurious level.
`levels[0]` is `"Tema"`, `levels[1]` is `"Accra"`, `levels[2]` is `"Kumasi"` — reading
`levels[code]` for any row recovers its original city.

Example 2. Sort levels lexicographically instead of by first appearance.

```cpp
auto sorted_factor = orders.factor("city", false, /*sort_levels=*/true);
std::cout << sorted_factor.codes;
std::cout << sorted_factor.levels;
```

### Output

```text
0  2
1  0
2  2
3  1
4  NULL
ptype: Series
```

```text
0  Accra
1  Kumasi
2  Tema
ptype: Note
```

The level order is now alphabetical (`Accra, Kumasi, Tema`), so the codes shift to
match: `Tema` is now `2` instead of `0`. `sort_levels` is the right choice when the
code assignment needs to be stable and reproducible across separately-built frames
that might encounter their distinct values in a different order.

## Notes

- `codes` carries no case labels or display name of its own; it prints with a plain
  `0, 1, 2, ...` index, the same as a fresh `Slate` built directly from a `Series`.
- A missing `feature` value always produces a `NULL` code — it is never assigned to an
  implicit "unknown" level, and it never appears in `levels`.
- [`encode(feature, EncodeMode::Ordinal, options)`](encode.md) is equivalent to
  `factor(feature, options.ordered, options.sort_levels).codes`, spliced back into the
  frame at `feature`'s original position — use `encode()` directly when you only need
  the codes back inside the frame and don't need `levels`.
- `CategoryEncoding::decode()` reverses the process: given `codes` and `levels`, it
  reconstructs a `Slate` equal to the original `feature` (missing codes decode back to
  `NULL`).

## See Also

- [`DataFrame::encode`](encode.md): the frame-level convenience wrapper — `Ordinal`
  mode wraps this method directly, and `OneHot` mode solves the same "make this
  categorical column numeric" problem for models that need indicator columns instead
  of integer codes.
- [`DataFrame::bucketize`](bucketize.md): discretize a *continuous* numeric feature
  into a small number of bucket codes, upstream of factoring or encoding.
- [`DataFrame::cast`](cast.md): change a feature's dtype or payload kind outright, for
  conversions that aren't about categorical levels at all.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extend.html">
    <span class="label">Previous</span>
    <span class="title">extend</span>
  </a>
  <a class="next-link" href="feataxis.html">
    <span class="label">Next</span>
    <span class="title">feataxis</span>
  </a>
</div>
