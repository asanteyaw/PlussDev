<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c6b83717d92a -->

# Slate::group

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate::group() const -> SlateGroupBuilder
Slate::group(const SlateGroupOptions& options) const -> SlateGroupBuilder
```

## What It Does

The entry point into `Slate`'s group-by pipeline. `group()` itself does no grouping — it
binds the current slate (as the value column to aggregate/transform) and an optional
`SlateGroupOptions` policy, and hands back a `SlateGroupBuilder` whose `.by(key)` method
supplies the actual grouping key(s) and produces a `SlateGrouped`:

### `SlateGroupOptions` fields

| Field | Type | Default |
|---|---|---|
| `dropnull` | `bool` | `true` |
| `sort` | `bool` | `false` |

### `SlateGroupBuilder` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `by` | Method | `SlateGrouped by(const Slate& key) const` |
| `by` | Method | `SlateGrouped by(std::initializer_list<Slate> keys) const` |
| `by` | Method | `SlateGrouped by(pluss::msharp::RangeView::Of<Slate> keys) const` |

### `SlateGrouped` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `agg` | Method | `DataFrame agg(std::initializer_list<SlateGroupSpec> specs) const` |
| `transform` | Method | `Slate transform(GroupOp op) const` |
| `transform` | Method | `DataFrame transform(std::initializer_list<SlateGroupSpec> specs) const` |
| `filter` | Method | `Slate filter(const Slate& keep) const` |

The full chain reads `slate.group(options).by(key).agg({...})` (one row per group) or
`.transform(op)` (one row per *input* row, broadcasting each group's aggregate back out) —
the same split `pandas`' `groupby().agg()` vs. `groupby().transform()` draws.

## Parameters

**options** (`const SlateGroupOptions&`, optional)
    `dropnull` (default `true`) excludes rows whose grouping key is `NULL` from every
    group entirely, rather than collecting them into their own `NULL` group. `sort`
    (default `false`) sorts the output by group key; when `false`, groups appear in
    first-occurrence order.

## Returns

`SlateGroupBuilder`
    A binder for the current slate (and `options`) — call `.by(key)` on it to actually
    form groups.

## Examples

Example 1. Group by a `Note`-kind key slate and aggregate each group to a single sum —
`agg()` returns a `DataFrame`, one row per group.

```cpp
using namespace pluss::table;

auto sales = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto region = Slate(pluss::make_note({"east", "west", "east", "west"}));

auto out = sales.group().by(region).agg({SlateGroupSpec{"total", GroupOp::Sum}});
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
auto sales = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto region = Slate(pluss::make_note({"east", "west", "east", "west"}));

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

Example 3. Pass `SlateGroupOptions{.sort = true}` to `group()` to sort the output by
group key, instead of the default first-occurrence order.

```cpp
SlateGroupOptions options;
options.sort = true;

auto values = Slate(pluss::make_series({10.0, 20.0, 30.0}));
auto key = Slate(pluss::make_note({"b", "a", "b"}));

auto out = values.group(options).by(key).agg({SlateGroupSpec{"mean_val", GroupOp::Mean}});
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

- `group()`/`group(options)` alone do no grouping — they only bind the value slate and
  options; `.by(key)` is what actually forms groups.
- `agg()` returns a `DataFrame` (one row per group); `transform(op)` returns a `Slate`
  the same length as the input (one row per original row, broadcasting each group's
  aggregate).
- `dropnull` (default `true`) excludes `NULL`-keyed rows from every group; `sort` (default
  `false`) controls whether groups appear in key order or first-occurrence order.
- `.by(...)` accepts a single key slate, or multiple keys (via `std::initializer_list` or
  a `RangeView`) for a composite group-by.

## See Also

- [`DataFrame::group`](../dataframe/group.md): the frame-level equivalent, for grouping
  and aggregating across several features at once.
- [`Slate::freq`](freq.md): a narrower, single-purpose alternative for just counting
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
