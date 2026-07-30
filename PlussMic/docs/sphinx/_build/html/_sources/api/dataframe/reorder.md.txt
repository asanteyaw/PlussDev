<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4d73c05d3bfd -->

# DataFrame::reorder

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::reorder(pluss::msharp::RangeView::Of<std::string_view> names) const -> DataFrame
DataFrame::reorder(const ReorderSpec& spec) const -> DataFrame
DataFrame::reorder(pluss::RangeView<ReorderSpec> specs) const -> DataFrame
DataFrame::reorder(std::initializer_list<ReorderSpec> specs) const -> DataFrame
```

## What It Does

Moves one or more named features into a new relative position, while every other
feature keeps its current relative order — without having to restate the frame's
entire column list the way [`features()`](features.md) reordering would. There are
two ways to describe a move:

- A plain list of names (`reorder({"city"})`) moves those features together to the
  **front**, preserving their relative order among themselves and the relative order
  of everything else. This is shorthand for a single `ReorderSpec` with
  `placement = Front`.
- A `ReorderSpec` (or several, applied left to right) supports front, back, before-an-
  anchor, or after-an-anchor placement, and can select the features to move either by
  an explicit name list or by a regex pattern matched against every feature name.

Each `ReorderSpec` in a multi-spec sequence sees the column order left behind by the
previous spec — later specs can reference columns by name regardless of where an
earlier spec moved them.

## Parameters

**names** (`pluss::RangeView<std::string_view>`)

Feature names to move together to the front, preserving their relative order among
themselves and the relative order of every untouched feature. Shorthand for a single
front-placement `ReorderSpec`.

**spec** / **specs** (`ReorderSpec` / a range or list of `ReorderSpec`)

One or more move instructions, each built via `ReorderSpec::move(...)` or
`ReorderSpec::move_regex(...)` and a placement (`.first()`, `.last()`,
`.before(anchor)`, `.after(anchor)`). Several specs are applied in the exact order
given, each seeing the previous spec's resulting order.

## Returns

`DataFrame`
    A new frame with the moved feature(s) repositioned as described; every untouched
    feature keeps its relative order (values, dtypes, and case labels are unchanged).

## Examples

Example 1. Move one feature to sit immediately after a named anchor.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto spec = DataFrame::ReorderSpec::move("sales").after("city");
auto reordered = orders.reorder(spec);
std::cout << reordered;
```

### Output

```text
==========================
     | id    city    sales
--------------------------
0    | 1     Accra   100  
1    | 2     Tema    120  
2    | 3     Kumasi  90   
--------------------------
[3 cs x 3 feat]
```

`"sales"` moves to sit right after `"city"`; `"id"` (not part of the move) keeps its
original front position.

Example 2. Apply a multi-step reorder plan, where each step sees the previous step's
result.

```cpp
using Move = DataFrame::ReorderSpec;
std::vector<Move> plan{
    Move::move({"city", "sales"}).first(),
    Move::move("id").after("sales"),
};
auto out = orders.reorder(plan);
std::cout << out;
```

### Output

```text
==========================
     | city    sales  id  
--------------------------
0    | Accra   100    1   
1    | Tema    120    2   
2    | Kumasi  90     3   
--------------------------
[3 cs x 3 feat]
```

The first step moves `{"city", "sales"}` to the front (in exactly that order,
regardless of their original position); the second step moves `"id"` to sit after
`"sales"` in the order that first step produced — since `"id"` was already last, this
step leaves the order unchanged.

Example 3. Move a subset of features to the front using the plain-name-list
shorthand.

```cpp
auto front_moved = orders.reorder({"city"});
std::cout << front_moved;
```

### Output

```text
==========================
     | city    id    sales
--------------------------
0    | Accra   1     100  
1    | Tema    2     120  
2    | Kumasi  3     90   
--------------------------
[3 cs x 3 feat]
```

`reorder({"city"})` is shorthand for `reorder(ReorderSpec::move("city").first())` —
`"city"` moves to the front, and `"id"`/`"sales"` keep their original relative order
behind it.

## Notes

- `reorder({"a", "b"})` moves `"a"` and `"b"` together to the front, preserving the
  relative order of every other feature — it does not mean "replace the entire
  feature order with just these two."
- `ReorderSpec::move_regex(pattern)` selects every feature whose name matches
  `pattern` (ECMAScript regex), instead of naming them explicitly — useful when
  moving a family of similarly-named columns (for example, `move_regex("^col_2[1-4]$")`).
- Every feature named in a `ReorderSpec` must exist exactly once across the frame; an
  unknown name, a duplicate within one spec, or a `before`/`after` anchor that is
  itself one of the moved features throws.

## See Also

- [`DataFrame::place`](place.md): move (or insert) exactly one feature relative to
  one anchor, a lighter-weight alternative when only a single feature needs to move.
- [`DataFrame::features`](features.md): select or drop features outright, rather than
  just reordering them.
- [`DataFrame::rename`](rename.md): change feature names, a distinct operation from
  reordering them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename_.html">
    <span class="label">Previous</span>
    <span class="title">rename_</span>
  </a>
  <a class="next-link" href="repeated.html">
    <span class="label">Next</span>
    <span class="title">repeated</span>
  </a>
</div>
