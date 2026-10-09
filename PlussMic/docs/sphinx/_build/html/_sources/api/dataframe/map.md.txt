<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ba3097eddc87 -->

# DataFrame::map

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::map(const std::function<SlateValue(const SlateValue&)>& fn) const -> DataFrame
```

## What It Does

Applies `fn` to every cell of every feature currently on the frame, one scalar value
at a time, and returns a new frame with those features replaced. `fn` receives one
cell as a `SlateValue` and must return one `SlateValue` back — it is skipped
entirely for missing cells (a `NULL` cell stays `NULL` without `fn` ever being
called), and every non-null cell in one feature's column must map to the **same**
output kind (you cannot have `fn` return a numeric value for one row and text for
another row of the same feature).

`map()` carries no feature selector of its own: it always applies `fn` to *every*
feature on the current frame, which only makes sense if `fn` can handle every
feature's kind present. Narrow to the feature(s) that need this particular `fn` first
with [`features()`](features.md) — `students.features({"score"}).map(fn)` — when
different features need different treatment, and recombine with
[`update()`](update.md) afterward if the untouched features need to survive alongside
the mapped result.

Unlike [`transform()`](transform.md), which operates on a whole feature `Slate` at
once (so it can do things like a running total), `map()` only ever sees one value at
a time — it is the right tool for a pure, independent per-cell computation.

## Parameters

**fn** (`const std::function<SlateValue(const SlateValue&)>&`)

The per-cell callback. Extract the incoming scalar with `.to_dbl()` / `.to_int()` /
`.to_str()` (matching the feature's kind), compute a new value, and wrap it back into
a `SlateValue` over a length-1 `Slate` (for example
`SlateValue(Slate(pluss::make_series({result})))`). The output kind does not have to
match the input kind — mapping `Note` cells to their string length (an `Int64`
result) is a valid, kind-changing use of `map()`.

## Returns

`DataFrame`
    A new frame, same shape as the (possibly `features()`-narrowed) source, with every
    feature replaced by `fn`'s output. Missing cells stay missing.

## Examples

Example 1. Square every value in one numeric feature.

```cpp
using namespace pluss::table;

auto students = DataFrame({
    {"score", Slate(pluss::make_series({80.0, 90.0, 70.0}))},
    {"city",  Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
std::cout << students;
```

### Output (before)

```text
====================
     | score  city  
--------------------
0    | 80     Accra 
1    | 90     Tema  
2    | 70     Kumasi
--------------------
[3 cs x 2 feat]
```

```cpp
auto score_squared = students.features({"score"}).map(
    [](const SlateValue& v) {
      const auto x = v.to_dbl();
      return SlateValue(Slate(pluss::make_series({x * x})));
    });
std::cout << score_squared;

auto squared = students.update(score_squared);
std::cout << squared;
```

### Output (narrowed, mapped-only result)

```text
========
     | score
--------
0    | 6400
1    | 8100
2    | 4900
--------
[3 cs x 1 feat]
```

### Output (merged back into the full frame)

```text
====================
     | score  city  
--------------------
0    | 6400   Accra 
1    | 8100   Tema  
2    | 4900   Kumasi
--------------------
[3 cs x 2 feat]
```

Only `score` changed (`80 -> 6400`, `90 -> 8100`, `70 -> 4900`); `city` was never
part of the narrowed frame `map()` ran against, so [`update()`](update.md) leaves it
untouched.

Example 2. A kind-changing map: replace text with its length, then splice the
kind-changed result back in with [`set()`](set.md) — not `update()`, since `update()`
only refreshes values within a feature's existing kind and cannot swap `Note` for
`Int64`.

```cpp
auto city_length = students.features({"city"}).map(
    [](const SlateValue& v) {
      const auto s = v.to_str();
      return SlateValue(Slate(pluss::make_series({static_cast<std::int64_t>(s.size())})));
    });

auto with_length = students;
with_length.set("city") = city_length;
std::cout << with_length;
```

### Output

```text
==================
     | score  city
------------------
0    | 80     5   
1    | 90     4   
2    | 70     6   
------------------
[3 cs x 2 feat]
```

`city` is now an `Int64` `Series` of string lengths (`"Accra"` -> `5`, `"Tema"` -> `4`,
`"Kumasi"` -> `6`) rather than the original `Note` text — `map()` does not require the
output kind to match the input kind, as long as every row of that one feature agrees
on the output kind. [`set(name)`](set.md) overwrites a feature outright regardless of
its previous kind, unlike [`update()`](update.md), which only refreshes values within
a matching kind.

## Notes

- `fn` is never invoked for a missing cell; the cell stays `NULL` in the output
  without needing special-case handling inside `fn` itself.
- Every non-null result within one feature must share the same `SlateKind`; mixing
  kinds within a single feature's output throws `"Slate::map: output kinds must be
  consistent"`.
- `map()` has no feature selector of its own and applies `fn` to literally every
  feature on the current frame, including features of different kinds — narrow with
  [`features()`](features.md) first unless `fn` is written to handle every kind
  present (typically by branching on `v.kind()` inside the lambda).
- `map()` itself is non-mutating; it always returns a new frame scoped to whatever
  features it was called on. Recombine a narrowed result with
  [`update()`](update.md) when the output kind matches the original, or with
  [`set()`](set.md) when `fn` changed the feature's kind (Example 2).

## See Also

- [`DataFrame::transform`](transform.md): apply a function to a whole feature
  `Slate` at once, for computations that need to see more than one cell (running
  totals, shifts, and so on).
- [`DataFrame::cast`](cast.md): change a feature's dtype or payload kind directly,
  when the target kind is fixed rather than computed per-cell.
- [`DataFrame::fold`](fold.md) / [`DataFrame::flow`](flow.md): the whole-frame
  callback hooks, for computations that need more than one feature at a time.
- [`DataFrame::features`](features.md): narrow the frame to the feature(s) `fn`
  should apply to.
- [`DataFrame::update`](update.md), [`DataFrame::set`](set.md): recombine a narrowed
  `map()` result back into the full frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lxor.html">
    <span class="label">Previous</span>
    <span class="title">lxor</span>
  </a>
  <a class="next-link" href="mask.html">
    <span class="label">Next</span>
    <span class="title">mask</span>
  </a>
</div>
