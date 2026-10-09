<!-- pluss-hand-authored -->

# Series::TraverseAccessor::find_first

[Back to Series::TraverseAccessor API Reference](../../series_traverse_accessor_api_reference.md)

## Signatures

```cpp
TraverseMatch Series::TraverseAccessor::find_first(const Series& mask,
                                                 std::int64_t dim = -1,
                                                 bool per_batch = false) const;
TraverseMatch SeriesTraverseProxy::find_first(const Series& mask,
                                              std::int64_t dim = -1,
                                              bool per_batch = false) const;
```

## What It Does

`find_first(mask)` keeps only the **first** matching position rather than every
match. "First" means earliest in row-major order (last axis fastest). Three
modes select how much of the `Series` shares one "first":

- **`find_first(mask)`** — one global winner. The result has `count() <= 1`.
- **`find_first(mask, dim)`** — one winner per *lane* along `dim`: the search
  collapses `dim`, so every distinct combination of the other axes gets its own
  first match. A lane with no match contributes nothing.
- **`find_first(mask, -1, /*per_batch=*/true)`** — one winner per index along
  the **leading** axis (axis `0`). Each "batch" row gets its own first match.

For a rank-2 `Series`, `find_first(mask, 1)` and `find_first(mask, -1, true)`
land on the same positions — one per row. They diverge at rank 3 and above,
where `dim` collapses a single named axis while `per_batch` groups only by
axis `0`.

Only one of `dim` and `per_batch` may be set at a time.

A typical use is "the first time each sequence in a batch crosses a threshold":
build the crossing mask, then `find_first(mask, -1, true)`.

## Parameters

**mask** (`const Series&`)

A condition shaped like the traversed `Series`. "Non-zero means true."

**dim** (`std::int64_t`, default `-1`)

The axis to collapse. `-1` (the default) means "no per-lane grouping" unless
`per_batch` is set. Negative values count from the end. Out of range is an
error.

**per_batch** (`bool`, default `false`)

When `true`, group by the leading axis instead of by `dim`. Requires rank `>=
1`, and cannot be combined with an explicit `dim`.

## Returns

[`TraverseMatch`](match.md)
    Row-major coordinates and matched values of the retained first positions.
    `count()` is `1` for the global mode, and at most one per lane / per batch
    otherwise.

## Gradient

Coordinates carry no gradient. `match.values()` and `match.select_from(other)`
are differentiable gathers.

## Examples

Example 1. The global first match, in row-major order.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto first = grid.traverse.find_first(grid.ge(6.0));

std::cout << "count=" << first.count() << "\n";
std::cout << first.coords();
std::cout << first.values();
```

### Output

```text
count=1
pluss::Series(shape=[1,2], strides=[2,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[0, 0]]
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[10]
```

Example 2. One winner per row with `dim = 1`.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto per_row = grid.traverse.find_first(grid.lt(5.0), 1);

std::cout << "count=" << per_row.count() << "\n";
std::cout << per_row.coords();
std::cout << per_row.values();
```

### Output

```text
count=3
pluss::Series(shape=[3,2], strides=[2,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[0, 1],
  [1, 0],
  [2, 1]]
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 2, 1]
```

Row `0`'s first value `< 5` is `4` at column `1`; row `1`'s is `2` at column
`0`; row `2`'s is `1` at column `1`.

Example 3. `per_batch` matches `dim = 1` on a rank-2 input.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

auto per_row  = grid.traverse.find_first(grid.ge(6.0), 1);
auto per_batch = grid.traverse.find_first(grid.ge(6.0), -1, /*per_batch=*/true);

std::cout << "dim=1  count=" << per_row.count() << "\n";
std::cout << "batch  count=" << per_batch.count() << "\n";
std::cout << "same coords=" << (per_row.coords().eq(per_batch.coords()).all().item<bool>() ? "yes" : "no");
```

### Output

```text
dim=1  count=3
batch  count=3
same coords=yes
```

Example 4. At rank 3 the two modes differ. Here a `2 x 2 x 3` cube of the
values `0..11`, matching `>= 3`.

```cpp
std::vector<double> d(12);
for (int i = 0; i < 12; ++i) d[i] = i;
auto cube = pluss::make_series(d, {2, 2, 3});
auto mask = cube.ge(3.0);

auto by_dim   = cube.traverse.find_first(mask, 2);                // collapse axis 2
auto by_batch = cube.traverse.find_first(mask, -1, /*per_batch=*/true);   // group by axis 0

std::cout << "by_dim count=" << by_dim.count() << "\n";
std::cout << by_dim.coords();
std::cout << "by_batch count=" << by_batch.count() << "\n";
std::cout << by_batch.coords();
```

### Output

```text
by_dim count=3
pluss::Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[[0, 1, 0],
  [1, 0, 0],
  [1, 1, 0]]
by_batch count=2
pluss::Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[0, 1, 0],
  [1, 0, 0]]
```

`dim = 2` gives one winner per `(axis0, axis1)` pair — three, because the pair
`(0, 0)` (values `0, 1, 2`) never reaches `3`. `per_batch` gives one winner per
`axis0` index — two.

Example 5. `dim` and `per_batch` cannot both be set.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

try {
  grid.traverse.find_first(grid.ge(6.0), 1, /*per_batch=*/true);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::traverse.find_first: choose either an explicit dim or per_batch=true
```

Example 6. An out-of-range `dim` is rejected.

```cpp
auto grid = pluss::make_series({
    {10.0, 4.0, 7.0},
    {2.0, 9.0, 6.0},
    {8.0, 1.0, 5.0},
});

try {
  grid.traverse.find_first(grid.ge(6.0), 5);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Series::traverse.find_first: dim out of range
```

## Gotchas And Failure Modes

- "First" is row-major order, last axis fastest — the same order the other
  finders report matches in.
- A lane or batch with no match simply produces no row; `count()` is not padded.
- `dim` and `per_batch` are mutually exclusive.
- `per_batch` needs rank `>= 1`.
- Coordinates are non-differentiable; `values()` and `select_from()` are.

## Summary

`find_first(mask [, dim] [, per_batch])` keeps only the earliest match:

- no argument: one global winner
- `dim`: one winner per lane along that axis
- `per_batch`: one winner per leading-axis index
- `dim` and `per_batch` cannot be combined

## See Also

- [`find_all`](find_all.md): keep every match instead of the first.
- [`Series::argmax`](../series/argmax.md): the first index of the maximum, a
  related "first position" idea for a different predicate.
- [`TraverseMatch`](match.md): the result object.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="find_eq.html">
    <span class="label">Previous</span>
    <span class="title">find_eq</span>
  </a>
  <a class="next-link" href="find_ge.html">
    <span class="label">Next</span>
    <span class="title">find_ge</span>
  </a>
</div>
