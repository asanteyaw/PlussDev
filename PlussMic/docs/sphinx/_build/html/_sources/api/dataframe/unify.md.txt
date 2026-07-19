<!-- pluss-hand-authored -->

# DataFrame::unify

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::unify(std::string_view left_on, const DataFrame& right, std::string_view right_on = "", std::string_view how = "common", std::string right_suffix = "_right", std::string_view validate = "many_to_many") const -> DataFrame
DataFrame::unify(pluss::RangeView<std::string_view> left_on, const DataFrame& right, pluss::RangeView<std::string_view> right_on = {}, std::string_view how = "common", std::string right_suffix = "_right", std::string_view validate = "many_to_many") const -> DataFrame
DataFrame::unify(std::initializer_list<std::string_view> left_on, const DataFrame& right, std::initializer_list<std::string_view> right_on = {}, std::string_view how = "common", std::string right_suffix = "_right", std::string_view validate = "many_to_many") const -> DataFrame
```

## What It Does

A relational, key-value join between this frame and `right` — matching rows by the
*values* in `left_on`/`right_on`, not by case label or row position. This is
distinct from [`combine()`](combine.md)/[`join()`](join.md)/[`align()`](align.md),
which all match rows by **case-label value**; `unify()` matches by an arbitrary key
feature (or several), the same shape as a SQL `JOIN` or a pandas `merge()`.

`right_on` defaults to reusing `left_on`'s names when left and right share the same
key column name(s); pass it explicitly when the two frames name their keys
differently.

## Parameters

**left_on** / **right_on**

Key feature name(s) on this frame / on `right` used to match rows. `right_on`
defaults (empty) to reusing `left_on`'s names.

**right** (`const DataFrame&`)

The right-hand frame to join against.

**how** (`std::string_view`, default `"common"`)

The join mode:

- `"common"` (aliases `"inner"`): keep only rows whose key values match on both
  sides.
- `"keep_left"` (aliases `"left"`): keep every row from this frame, attaching a
  match from `right` where one exists (`NULL` on `right`'s columns otherwise).
- `"keep_right"` (aliases `"right"`): the mirror image, keeping every row from
  `right`.
- `"unite"` (aliases `"outer"`/`"full"`): keep the union of keys from both sides.
- `"cross"`: the cartesian product of every row in both frames; `left_on`/`right_on`
  must both be empty for this mode.
- `"left_only"` (alias `"left_anti"`): keep only this frame's rows whose key does
  **not** occur on the right — an anti-join.
- `"right_only"`: the mirror image anti-join, keeping only `right`'s unmatched rows.

**right_suffix** (`std::string`, default `"_right"`)

Suffix appended to a right-hand, non-key feature name that would otherwise collide
with an existing left-hand feature name.

**validate** (`std::string_view`, default `"many_to_many"`)

An optional key-cardinality check: `"many_to_many"` (no constraint, the default),
`"one_to_one"` (each key must appear at most once on both sides), `"one_to_many"`
(unique on the left, duplicates allowed on the right), or `"many_to_one"` (the
mirror image). A violated constraint throws.

## Returns

`DataFrame`
    The joined frame, following the requested `how` and `validate` rules. Case
    labels are not preserved from either input — the result gets a fresh default
    case index.

## Examples

Example 1. The default `"common"` (inner) join: only matching keys survive.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"customer_id", Thread(pluss::make_series({1, 2, 3}))},
    {"amount",      Thread(pluss::make_series({100.0, 200.0, 150.0}))},
});
std::cout << orders;

auto customers = DataFrame({
    {"customer_id", Thread(pluss::make_series({2, 3, 4}))},
    {"name",        Thread(pluss::make_note({"Bola", "Chen", "Dede"}))},
});
std::cout << customers;
```

### Output

```text
==========================
     | customer_id  amount
--------------------------
0    | 1            100   
1    | 2            200   
2    | 3            150   
--------------------------
[3 cs x 2 feat]
========================
     | customer_id  name
------------------------
0    | 2            Bola
1    | 3            Chen
2    | 4            Dede
------------------------
[3 cs x 2 feat]
```

```cpp
auto matched = orders.unify("customer_id", customers);
std::cout << matched;
```

### Output

```text
================================
     | customer_id  amount  name
--------------------------------
0    | 2            200     Bola
1    | 3            150     Chen
--------------------------------
[2 cs x 3 feat]
```

`customer_id = 1` (only in `orders`) and `customer_id = 4` (only in `customers`)
both disappear — the default `"common"` mode keeps only keys present on **both**
sides. The result carries a fresh `0`, `1` case index, unrelated to either input's
own case labels.

## Notes

- `unify()` matches rows by key **value**, never by case label or row position —
  this is the key distinction from [`combine()`](combine.md),
  [`join()`](join.md), and [`align()`](align.md).
- Repeated key values expand to every matching pair on the opposing side — a key
  that appears twice on the left and three times on the right produces six paired
  rows in the joined result (subject to `how`).
- `right_suffix` is only ever applied to right-hand, **non-key** feature names that
  collide with an existing left-hand name — the shared key column itself is never
  suffixed.
- The result's case labels are always a fresh default index, regardless of `how` —
  neither input frame's case labels carry through.

## See Also

- [`DataFrame::combine`](combine.md), [`DataFrame::join`](join.md),
  [`DataFrame::update`](update.md): the case-label-matched family of merges, a
  different alignment model from this key-value join.
- [`DataFrame::align`](align.md): the lower-level case-label alignment primitive
  those methods build on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ts.html">
    <span class="label">Previous</span>
    <span class="title">ts</span>
  </a>
  <a class="next-link" href="unsqueeze.html">
    <span class="label">Next</span>
    <span class="title">unsqueeze</span>
  </a>
</div>
