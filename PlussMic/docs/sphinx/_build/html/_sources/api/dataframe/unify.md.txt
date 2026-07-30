<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b43ad493f0d4 -->

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

The join mode. Matching is case-insensitive, and an empty string is treated the same
as `"common"`:

- `"common"` (aliases `"inner"`): keep only rows whose key values match on both
  sides.
- `"keep_left"` (aliases `"keep-left"`, `"left"`): keep every row from this frame,
  attaching a match from `right` where one exists (`NULL` on `right`'s columns
  otherwise).
- `"keep_right"` (aliases `"keep-right"`, `"right"`): the mirror image, keeping every
  row from `right`.
- `"unite"` (aliases `"outer"`/`"full"`): keep the union of keys from both sides.
- `"cross"`: the cartesian product of every row in both frames; `left_on`/`right_on`
  must both be empty for this mode.
- `"left_only"` (aliases `"left-only"`, `"left_anti"`): keep only this frame's rows
  whose key does **not** occur on the right — an anti-join.
- `"right_only"` (aliases `"right-only"`, `"right_anti"`): the mirror image anti-join,
  keeping only `right`'s unmatched rows.

Passing anything else throws with the message
`"DataFrame::unify: supported modes are 'common', 'keep_left', 'keep_right', 'unite', 'cross', 'left_only', and 'right_only'"`
(the canonical names only — the message itself doesn't list the shorthand aliases
above, even though they're all accepted).

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

The dataset used throughout — `id = 1` exists only in `customers`, `id = 4` exists
only in `orders`, and `2`/`3` exist on both sides:

```cpp
using namespace pluss::table;

auto customers = DataFrame({
    {"id",   Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"name", Thread(pluss::make_note({"Ada", "Bola", "Chen"}))},
});
std::cout << customers;

auto orders = DataFrame({
    {"id",     Thread(pluss::make_series({2.0, 3.0, 4.0}))},
    {"amount", Thread(pluss::make_series({100.0, 200.0, 300.0}))},
});
std::cout << orders;
```

### Output (customers)

```text
================
      | id  name
----------------
0     | 1   Ada 
1     | 2   Bola
2     | 3   Chen
----------------
[3 cs x 2 feat]
```

### Output (orders)

```text
==================
      | id  amount
------------------
0     | 2   100   
1     | 3   200   
2     | 4   300   
------------------
[3 cs x 2 feat]
```

Example 1. The default `how = "common"` (inner join): only keys present on **both**
sides survive.

```cpp
auto common = customers.unify("id", orders);
std::cout << common;
```

### Output

```text
========================
      | id  name  amount
------------------------
0     | 2   Bola  100   
1     | 3   Chen  200   
------------------------
[2 cs x 3 feat]
```

`id = 1` (only in `customers`) and `id = 4` (only in `orders`) both disappear. Since
`left_on` and `right_on` share the name `"id"`, only one `id` column survives in the
output — the right side's copy of the shared key is dropped, not merged separately.

Example 2. `how = "keep_left"`: every `customers` row survives; unmatched cells from
`orders` become `NULL`.

```cpp
auto keep_left = customers.unify("id", orders, "id", "keep_left");
std::cout << keep_left;
```

### Output

```text
========================
      | id  name  amount
------------------------
0     | 1   Ada   NULL  
1     | 2   Bola  100   
2     | 3   Chen  200   
------------------------
[3 cs x 3 feat]
```

`id = 1` survives with `amount = NULL` since no `orders` row has that key — the
mirror image of Example 1 dropping it entirely.

Example 3. `how = "keep_right"`: every `orders` row survives instead; unmatched cells
from `customers` become `NULL`.

```cpp
auto keep_right = customers.unify("id", orders, "id", "keep_right");
std::cout << keep_right;
```

### Output

```text
========================
      | id  name  amount
------------------------
0     | 2   Bola  100   
1     | 3   Chen  200   
2     | 4   NULL  300   
------------------------
[3 cs x 3 feat]
```

Now `id = 4` survives (from `orders`) with `name = NULL`, and `id = 1` (only in
`customers`) is gone.

Example 4. `how = "unite"`: every key from **either** side survives — the union.

```cpp
auto united = customers.unify("id", orders, "id", "unite");
std::cout << united;
```

### Output

```text
========================
      | id  name  amount
------------------------
0     | 1   Ada   NULL  
1     | 2   Bola  100   
2     | 3   Chen  200   
3     | 4   NULL  300   
------------------------
[4 cs x 3 feat]
```

Both of Examples 2 and 3's "only on one side" rows (`id = 1` and `id = 4`) survive
together here, each `NULL` on whichever side didn't have them. `customers`'s own rows
come first, in their original order, followed by any `orders`-only rows.

Example 5. `how = "left_only"`: an anti-join — keep only `customers` rows with **no**
match in `orders`.

```cpp
auto left_only = customers.unify("id", orders, "id", "left_only");
std::cout << left_only;
```

### Output

```text
========================
      | id  name  amount
------------------------
0     | 1   Ada   NULL  
------------------------
[1 cs x 3 feat]
```

Only `id = 1` qualifies — every other `customers` row has a match in `orders` and is
excluded. `amount` still appears as a column (structurally identical to a
`"keep_left"` row), just `NULL` throughout, since there's never a match to fill it.

Example 6. `how = "right_only"`: the mirror-image anti-join — `orders` rows with no
match in `customers`.

```cpp
auto right_only = customers.unify("id", orders, "id", "right_only");
std::cout << right_only;
```

### Output

```text
========================
      | id  name  amount
------------------------
0     | 4   NULL  300   
------------------------
[1 cs x 3 feat]
```

Example 7. `how = "cross"`: the cartesian product of every row in both frames.
`left_on`/`right_on` must both be empty for this mode — there is no key to match on,
every row pairs with every row.

```cpp
auto small_customers = DataFrame({
    {"id",   Thread(pluss::make_series({1.0, 2.0}))},
    {"name", Thread(pluss::make_note({"Ada", "Bola"}))},
});
auto small_orders = DataFrame({
    {"id",     Thread(pluss::make_series({2.0, 3.0}))},
    {"amount", Thread(pluss::make_series({100.0, 200.0}))},
});

auto crossed = small_customers.unify({}, small_orders, {}, "cross");
std::cout << crossed;
```

### Output

```text
==================================
      | id  name  id_right  amount
----------------------------------
0     | 1   Ada   2         100   
1     | 1   Ada   3         200   
2     | 2   Bola  2         100   
3     | 2   Bola  3         200   
----------------------------------
[4 cs x 4 feat]
```

`2` customers × `2` orders gives `4` result rows. With no join keys at all, `id` is no
longer a *shared* key column to dedupe — both frames' own `id` columns survive, and
since the name collides, `orders`'s copy is automatically renamed `id_right` (the
default `right_suffix`).

Example 8. `right_suffix` applies to **any** colliding non-key column name, not just
`"cross"`'s unkeyed columns — here both frames happen to have their own `"name"`
feature.

```cpp
auto orders_with_names = DataFrame({
    {"id",     Thread(pluss::make_series({2.0, 3.0, 4.0}))},
    {"name",   Thread(pluss::make_note({"Ba", "Ch", "Da"}))},
    {"amount", Thread(pluss::make_series({100.0, 200.0, 300.0}))},
});

auto default_suffix = customers.unify("id", orders_with_names);
std::cout << default_suffix;

auto custom_suffix = customers.unify("id", orders_with_names, "id", "common", "_o");
std::cout << custom_suffix;
```

### Output (default `right_suffix = "_right"`)

```text
====================================
      | id  name  amount  name_right
------------------------------------
0     | 2   Bola  100     Ba        
1     | 3   Chen  200     Ch        
------------------------------------
[2 cs x 4 feat]
```

### Output (`right_suffix = "_o"`)

```text
================================
      | id  name  amount  name_o
--------------------------------
0     | 2   Bola  100     Ba    
1     | 3   Chen  200     Ch    
--------------------------------
[2 cs x 4 feat]
```

`name` (the shared key in Example 1) is never suffixed — only *non-key* collisions
go through `right_suffix`, and here it's `orders_with_names`'s own `name` feature
that collides with `customers`'s `name`, not a join key at all.

Example 9. `validate = "one_to_one"` throws when either side has a repeated key —
here `customers` has `id = 2` twice.

```cpp
auto dup_customers = DataFrame({
    {"id",   Thread(pluss::make_series({2.0, 2.0, 3.0}))},
    {"name", Thread(pluss::make_note({"Ada", "Bola", "Chen"}))},
});

try {
  auto bad = dup_customers.unify("id", orders, "id", "common", "_right", "one_to_one");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::unify: validate='one_to_one' requires unique left keys
```

The default `"many_to_many"` allows repeated keys on either side freely; `"one_to_one"`,
`"one_to_many"` (unique left required), and `"many_to_one"` (unique right required)
each enforce a cardinality constraint and throw the moment it's violated.

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
