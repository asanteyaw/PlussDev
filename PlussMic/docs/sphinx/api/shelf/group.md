<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c6b83717d92a -->

# Shelf::group

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
ShelfGroupBuilder Shelf::group() const
ShelfGroupBuilder Shelf::group(const ShelfGroupOptions& options) const
```

## What It Does

The entry point into `Shelf`'s group-by pipeline. `group()` itself does no grouping — it
binds the current shelf (as the value column to aggregate/transform) and an optional
`ShelfGroupOptions` policy, and hands back a `ShelfGroupBuilder` whose `.by(key)` method
supplies the actual grouping key(s) and produces a `ShelfGrouped`:

### `ShelfGroupOptions` fields

| Field | Type | Default |
|---|---|---|
| `dropnull` | `bool` | `true` |
| `sort` | `bool` | `false` |

### `ShelfGroupBuilder` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `by` | Method | `ShelfGrouped by(const Shelf& key) const` |
| `by` | Method | `ShelfGrouped by(std::initializer_list<Shelf> keys) const` |
| `by` | Method | `ShelfGrouped by(pluss::msharp::RangeView::Of<Shelf> keys) const` |

### `ShelfGrouped` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `agg` | Method | `DataFrame agg(std::initializer_list<ShelfGroupSpec> specs) const` |
| `transform` | Method | `Shelf transform(GroupOp op) const` |
| `transform` | Method | `DataFrame transform(std::initializer_list<ShelfGroupSpec> specs) const` |
| `filter` | Method | `Shelf filter(const Shelf& keep) const` |

The full chain reads `shelf.group(options).by(key).agg({...})` (one row per group) or
`.transform(op)` (one row per *input* row, broadcasting each group's aggregate back out) —
the same split `pandas`' `groupby().agg()` vs. `groupby().transform()` draws.

## Parameters

**options** (`const ShelfGroupOptions&`, optional)
    `dropnull` (default `true`) excludes rows whose grouping key is `NULL` from every
    group entirely, rather than collecting them into their own `NULL` group. `sort`
    (default `false`) sorts the output by group key; when `false`, groups appear in
    first-occurrence order.

## Returns

`ShelfGroupBuilder`
    A binder for the current shelf (and `options`) — call `.by(key)` on it to actually
    form groups.

## Examples

Example 1. Group by a `Note`-kind key shelf and aggregate each group to a single sum —
`agg()` returns a `DataFrame`, one row per group.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west"}));

auto out = sales.group().by(region).agg({ShelfGroupSpec{"total", GroupOp::Sum}});
std::cout << out;
```

### Output

```text
==================
     | key   total
------------------
0    | east  40
1    | west  60
------------------
[2 cs x 2 feat]
```

Example 2. `transform()` broadcasts each group's aggregate back to every row of the
*original* length, instead of collapsing to one row per group.

```cpp
auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west"}));

auto out = sales.group().by(region).transform(GroupOp::Sum);
std::cout << out;
```

### Output

```text
0  40
1  60
2  40
3  60
ptype: Series
```

Every `"east"` row shows `40` (the sum of both east rows, `10 + 30`), and every `"west"`
row shows `60` (`20 + 40`) — the group's aggregate, not each row's own value.

Example 3. Pass `ShelfGroupOptions{.sort = true}` to `group()` to sort the output by
group key, instead of the default first-occurrence order.

```cpp
ShelfGroupOptions options;
options.sort = true;

auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0}));
auto key = Shelf(pluss::make_note({"b", "a", "b"}));

auto out = values.group(options).by(key).agg({ShelfGroupSpec{"mean_val", GroupOp::Mean}});
std::cout << out;
```

### Output

```text
=====================
     | key   mean_val
---------------------
0    | a     20
1    | b     20
---------------------
[2 cs x 2 feat]
```

`"a"` sorts before `"b"`, so it appears first — without `sort = true`, the output would
follow the key's first-occurrence order (`"b"` first, since it appears in row `0`).

## Notes

- `group()`/`group(options)` alone do no grouping — they only bind the value shelf and
  options; `.by(key)` is what actually forms groups.
- `agg()` returns a `DataFrame` (one row per group); `transform(op)` returns a `Shelf`
  the same length as the input (one row per original row, broadcasting each group's
  aggregate).
- `dropnull` (default `true`) excludes `NULL`-keyed rows from every group; `sort` (default
  `false`) controls whether groups appear in key order or first-occurrence order.
- `.by(...)` accepts a single key shelf, or multiple keys (via `std::initializer_list` or
  a `RangeView`) for a composite group-by.

## See Also

- [`DataFrame::group`](../dataframe/group.md): the frame-level equivalent, for grouping
  and aggregating across several features at once.
- [`Shelf::freq`](freq.md): a narrower, single-purpose alternative for just counting
  occurrences per distinct value, without the full group-by pipeline.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ge.html">
    <span class="label">Previous</span>
    <span class="title">ge</span>
  </a>
  <a class="next-link" href="gt.html">
    <span class="label">Next</span>
    <span class="title">gt</span>
  </a>
</div>
