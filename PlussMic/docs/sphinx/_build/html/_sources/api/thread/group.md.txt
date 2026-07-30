<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a0ab177155c4 -->

# Thread::group

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::group() const -> ThreadGroupBuilder
Thread::group(const ThreadGroupOptions& options) const -> ThreadGroupBuilder
```

## What It Does

The entry point into `Thread`'s group-by pipeline. `group()` itself does no grouping — it
binds the current thread (as the value column to aggregate/transform) and an optional
`ThreadGroupOptions` policy, and hands back a `ThreadGroupBuilder` whose `.by(key)` method
supplies the actual grouping key(s) and produces a `ThreadGrouped`:

```cpp
struct ThreadGroupOptions {
  bool dropnull{true};  // exclude rows whose group key is NULL
  bool sort{false};     // sort output by group key
};

class ThreadGroupBuilder {
public:
  ThreadGrouped by(const Thread& key) const;
  ThreadGrouped by(std::initializer_list<Thread> keys) const;
  ThreadGrouped by(pluss::msharp::RangeView::Of<Thread> keys) const;
};

class ThreadGrouped {
public:
  DataFrame agg(std::initializer_list<ThreadGroupSpec> specs) const;
  Thread transform(GroupOp op) const;
  DataFrame transform(std::initializer_list<ThreadGroupSpec> specs) const;
  Thread filter(const Thread& keep) const;
  // ...
};
```

The full chain reads `thread.group(options).by(key).agg({...})` (one row per group) or
`.transform(op)` (one row per *input* row, broadcasting each group's aggregate back out) —
the same split `pandas`' `groupby().agg()` vs. `groupby().transform()` draws.

## Parameters

**options** (`const ThreadGroupOptions&`, optional)
    `dropnull` (default `true`) excludes rows whose grouping key is `NULL` from every
    group entirely, rather than collecting them into their own `NULL` group. `sort`
    (default `false`) sorts the output by group key; when `false`, groups appear in
    first-occurrence order.

## Returns

`ThreadGroupBuilder`
    A binder for the current thread (and `options`) — call `.by(key)` on it to actually
    form groups.

## Examples

Example 1. Group by a `Note`-kind key thread and aggregate each group to a single sum —
`agg()` returns a `DataFrame`, one row per group.

```cpp
using namespace pluss::table;

auto sales = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto region = Thread(pluss::make_note({"east", "west", "east", "west"}));

auto out = sales.group().by(region).agg({ThreadGroupSpec{"total", GroupOp::Sum}});
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
auto sales = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto region = Thread(pluss::make_note({"east", "west", "east", "west"}));

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

Example 3. Pass `ThreadGroupOptions{.sort = true}` to `group()` to sort the output by
group key, instead of the default first-occurrence order.

```cpp
ThreadGroupOptions options;
options.sort = true;

auto values = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto key = Thread(pluss::make_note({"b", "a", "b"}));

auto out = values.group(options).by(key).agg({ThreadGroupSpec{"mean_val", GroupOp::Mean}});
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

- `group()`/`group(options)` alone do no grouping — they only bind the value thread and
  options; `.by(key)` is what actually forms groups.
- `agg()` returns a `DataFrame` (one row per group); `transform(op)` returns a `Thread`
  the same length as the input (one row per original row, broadcasting each group's
  aggregate).
- `dropnull` (default `true`) excludes `NULL`-keyed rows from every group; `sort` (default
  `false`) controls whether groups appear in key order or first-occurrence order.
- `.by(...)` accepts a single key thread, or multiple keys (via `std::initializer_list` or
  a `RangeView`) for a composite group-by.

## See Also

- [`DataFrame::group`](../dataframe/group.md): the frame-level equivalent, for grouping
  and aggregating across several features at once.
- [`Thread::freq`](freq.md): a narrower, single-purpose alternative for just counting
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
