<!-- pluss-hand-authored -->

# Series::slice_scatter

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::slice_scatter(const Series& src, std::int64_t dim, std::int64_t start, std::int64_t end) const
Series Series::slice_scatter(const Series& src, std::int64_t dim, std::int64_t start, std::int64_t end, std::int64_t step) const
```

## What It Does

Returns a **copy** of the receiver with one regular slice replaced by `src`.
The receiver is never modified — this is the functional (non-`_`) form.

The replaced positions along `dim` are the same half-open, optionally
stepped sequence [`slice`](slice.md) uses:

```text
start, start + step, start + 2*step, ...   while position < end
```

`end` is excluded. `slice_scatter(src, 1, 1, 5, 2)` replaces columns `1` and
`3`; every position outside that set keeps its value from the receiver.

`slice_scatter` is the write-side partner of [`slice`](slice.md): reading the
same slice back off the result returns `src`.

```text
result = base.slice_scatter(src, dim, start, end, step)
result.slice(dim, start, end, step) == src
```

Compared to its neighbors: [`select_scatter`](select_scatter.md) replaces
exactly one slice and *removes* the axis; [`index_copy`](index_copy.md)
replaces an arbitrary list of positions given by an index tensor.
`slice_scatter` is the one for a *regular* (arithmetic-progression) run.

## Parameters

**src** (`const Series&`): the complete replacement for the selected slice.
Same rank, dtype, and platform as the receiver; `src.size(dim)` equals the
number of selected positions, and every other axis matches the receiver
exactly. **No broadcasting** — `[2, 2]`, `[2, 1]`, and `[1, 2]` are distinct
contracts.

**dim** (`std::int64_t`): the axis to slice. Negative values count from the
end.

**start** (`std::int64_t`): the first selected position. Negative values are
taken relative to the axis length, then clamped into `[0, size]`.

**end** (`std::int64_t`): the exclusive upper bound, normalized and clamped
the same way. Must end up `>= start`.

**step** (`std::int64_t`, default `1` via the four-argument overload): the
positive spacing between selected positions. `0` and negatives raise.

## Returns

`Series`
    A new tensor with the receiver's shape, dtype, and platform. Neither the
    receiver nor `src` is modified. There is no public `slice_scatter_`.

## Gradient

Differentiable in both operands. Positions outside the slice still come from
the receiver, so their sensitivity passes back to it; replaced positions no
longer depend on the receiver and give it zero. `src`'s sensitivity is the
upstream sensitivity sliced with the same `dim`, `start`, `end`, `step`. This
clean split is a reason to prefer `slice_scatter` over mutating a temporary
inside a differentiable region.

## Examples

Example 1. The simplest case: replace a contiguous run of a 1-D receiver.

```cpp
auto base = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
auto src = pluss::make_series({90.0, 91.0, 92.0});
pluss::series::print(std::cout, base.slice_scatter(src, 0, 1, 4));
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0.0000, 90.0000, 91.0000, 92.0000, 4.0000, 5.0000]
```

Positions `1..3` take the three `src` values; `0`, `4`, `5` are untouched.

Example 2. A stepped slice — replace every second column in a bounded
interval.

```cpp
auto base = pluss::make_series({{0, 1, 2, 3, 4, 5}, {6, 7, 8, 9, 10, 11}});
auto src = pluss::make_series({{100, 200}, {300, 400}});
pluss::series::print(std::cout, base.slice_scatter(src, 1, 1, 5, 2));
```

### Output

```text
Series(shape=[2,6], strides=[6,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=
⎡ 0 100 2 200  4  5 ⎤
⎣ 6 300 8 400 10 11 ⎦
```

The two `src` columns map in order onto destination columns `1` and `3`.
Columns `0`, `2`, `4`, `5` still come from `base`.

Example 3. Omitting `step` selects every position; `dim = -1` is the last
axis.

```cpp
auto tail = pluss::make_series({{90, 91}, {92, 93}});
pluss::series::print(std::cout, base.slice_scatter(tail, -1, 4, 6));
```

### Output

```text
Series(shape=[2,6], strides=[6,1], dtype=int64, platform=cpu, numel=12, contiguous=true)
data=
⎡ 0 1 2 3 90 91 ⎤
⎣ 6 7 8 9 92 93 ⎦
```

Example 4. It round-trips with [`slice`](slice.md) — reading the same slice
off the result returns `src`.

```cpp
auto base = pluss::make_series({{0.0, 1.0, 2.0, 3.0, 4.0},
                                {5.0, 6.0, 7.0, 8.0, 9.0}});
auto src = pluss::make_series({{-1.0, -2.0}, {-3.0, -4.0}});
auto out = base.slice_scatter(src, 1, 1, 3);
std::cout << "slice back == src: " << std::boolalpha
          << out.slice(1, 1, 3).equal(src) << "\n";
```

### Output

```text
slice back == src: true
```

Example 5. Gradient: the reverse pass separates retained and replaced
positions.

```cpp
auto base = pluss::make_series({{0.0, 1.0, 2.0, 3.0}, {4.0, 5.0, 6.0, 7.0}});
auto src = pluss::make_series({{10.0, 20.0}, {30.0, 40.0}});
base.tracks_slope_(true);
src.tracks_slope_(true);
base.slice_scatter(src, 1, 1, 3).sum().reverse();
std::cout << "dbase = "; pluss::series::print(std::cout, base.sensitivity());
std::cout << "dsrc  = "; pluss::series::print(std::cout, src.sensitivity());
```

### Output

```text
dbase = Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=
⎡ 1.0000 0.0000 0.0000 1.0000 ⎤
⎣ 1.0000 0.0000 0.0000 1.0000 ⎦dsrc  = Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 1.0000 1.0000 ⎤
⎣ 1.0000 1.0000 ⎦
```

Columns `1` and `2` were overwritten, so `base` gets zero there; every `src`
value appears once in the sum and gets sensitivity `1`.

Example 6. Contract violations: a wrong `src` shape, a non-positive `step`,
and a dtype mismatch.

```cpp
auto base = pluss::make_series({{0, 1, 2, 3}, {4, 5, 6, 7}});
try {
  base.slice_scatter(pluss::make_series({1, 2, 3}), 1, 0, 2);   // slice is [2,2]
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  base.slice_scatter(pluss::make_series({{1, 2}, {3, 4}}), 1, 0, 4, 0);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
try {
  base.slice_scatter(pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}), 1, 0, 2);
} catch (const std::exception& e) {
  std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
slice_scatter: src shape must match the selected slice of self
slice_scatter: step must be > 0
slice_scatter: self and src must have the same dtype
```

Example 7. Clamped bounds can select nothing — then `src` must have size `0`
on `dim` and the result equals the receiver.

```cpp
auto base = pluss::make_series({{0, 1, 2}, {3, 4, 5}});
auto empty_src = pluss::series::zeros({2, 0}, pluss::ScalarType::Int64);
pluss::series::print(std::cout, base.slice_scatter(empty_src, 1, 5, 9));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 0 1 2 ⎤
⎣ 3 4 5 ⎦
```

`start = 5` and `end = 9` both clamp to `3` (the axis length), so no position
is selected.

## Gotchas And Failure Modes

- `end` is exclusive; `step` must be positive.
- `src` shape must equal the selected slice **exactly** — no broadcasting,
  no automatic singleton axis.
- `src` keeps the sliced axis (its `size(dim)` is the position count). If
  you want the axis *removed*, that is [`select_scatter`](select_scatter.md).
- Negative `start` / `end` are relative-then-clamped; over-large values clamp
  to the axis length and can yield an empty slice.
- Receiver and `src` must share dtype and platform.
- Functional only — there is no `slice_scatter_`.

## Summary

`slice_scatter(src, dim, start, end, step = 1)` returns the receiver with one
regular slice replaced:

- replaced positions are the half-open stepped range `[start, end)`; the rest
  are unchanged
- `src` matches the slice shape exactly (axis retained); no broadcasting
- round-trips with [`slice`](slice.md); differentiable, with a clean
  retained/replaced sensitivity split
- functional — never mutates the receiver

## See Also

- [`Series::slice`](slice.md): read the regular slice this writes.
- [`Series::select_scatter`](select_scatter.md): replace one slice and drop
  the axis.
- [`Series::index_copy`](index_copy.md): replace an arbitrary list of slices
  named by an index tensor.
- [`Series::index_copy_`](index_copy.md): an actual in-place slice update.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="softmax.html">
    <span class="label">Next</span>
    <span class="title">softmax</span>
  </a>
</div>
