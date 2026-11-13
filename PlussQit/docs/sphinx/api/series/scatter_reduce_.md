<!-- pluss-hand-authored -->

# Series::scatter_reduce_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::scatter_reduce_(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    const std::string& reduce)

Series& Series::scatter_reduce_(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    const std::string& reduce,
    bool include_self)

Series& Series::scatter_reduce_(
    std::int64_t dim,
    const Series& index,
    const Series& src,
    const std::string& reduce,
    bool include_self,
    bool has_initial,
    double initial)
```

## What It Does

`scatter_reduce_()` performs the same indexed reduction as
[`scatter_reduce()`](scatter_reduce.md), but it writes the result back into the
current `Series` instead of returning a separate copy.

That means the semantic questions are the same:

- which axis does `dim` address?
- which destinations are named by `index`?
- how do repeated hits combine?
- does `include_self` let the old value participate?
- is an explicit `initial` seed active?

The practical difference is mutation. After the call returns, the current
`Series` holds the reduced result.

## Parameters

The parameters have the same meanings as on
[`Series::scatter_reduce`](scatter_reduce.md):

- `dim` chooses the destination axis
- `index` supplies integral destination positions
- `src` supplies the reducing values
- `reduce` must be `"sum"`, `"mean"`, `"amax"`, or `"amin"`
- `include_self` decides whether the existing destination lane participates
- `has_initial` and `initial` activate an explicit seed

## Returns

`Series&`
    A reference to the mutated `Series`, so the call can be chained.

## Examples

Example 1. With `include_self = false` and an explicit seed, the current
`Series` becomes the seeded reduction result.

```cpp
auto values = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto index = pluss::make_series({1LL, 1LL});
auto src = pluss::make_series({1.0, 3.0});

values.scatter_reduce_(0, index, src, "sum", false, true, 5.0);
std::cout << values;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[5, 9, 5, 5]
```

Each destination starts from `5`, then destination `1` receives `1 + 3`.

Example 2. With `include_self = true`, the previous lane contents remain part
of the reduced value.

```cpp
auto keep_self = pluss::make_series({10.0, 20.0, 30.0, 40.0});

keep_self.scatter_reduce_(0, index, src, "sum", true);
pluss::series::print(std::cout, keep_self) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10.0000, 24.0000, 30.0000, 40.0000]
```

Example 3. In-place `"mean"` mutates the current value to the reduced average.

```cpp
auto mean_values = pluss::make_series({10.0, 20.0, 30.0, 40.0});

mean_values.scatter_reduce_(0, index, src, "mean", true);
std::cout << mean_values;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 8, 30, 40]
```

Destination `1` becomes `(20 + 1 + 3) / 3 = 8`.

Example 4. Negative dims work here too, but the result is written back into the
original matrix.

```cpp
auto matrix = pluss::make_series({
    {10.0, 20.0, 30.0},
    {40.0, 50.0, 60.0},
});
auto matrix_index = pluss::make_series({
    {2LL, 0LL},
    {1LL, 1LL},
});
auto matrix_src = pluss::make_series({
    {1.0, 2.0},
    {3.0, 4.0},
});

matrix.scatter_reduce_(-1, matrix_index, matrix_src, "sum", true);
std::cout << matrix;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[12, 20, 31],
  [40, 57, 60]]
```

Example 5. `"mean"` does not accept an explicit `initial` seed.

```cpp
auto bad_mean = pluss::make_series({10.0, 20.0, 30.0, 40.0});

try {
  bad_mean.scatter_reduce_(0, index, src, "mean", false, true, 5.0);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
scatter_reduce_: mean with explicit initial is unsupported
```

## Gotchas And Failure Modes

- This method mutates the current `Series`.
- The same reduction-name restrictions as `scatter_reduce()` apply.
- `include_self` determines whether the pre-existing value competes in the
  reduction before mutation finishes.
- `"mean"` with explicit `initial` fails.
- If you need the reduced result without mutating `self`, use
  [`scatter_reduce()`](scatter_reduce.md).

## Summary

Use `scatter_reduce_()` when the indexed reduction itself is correct, but the
result should land directly back into the current `Series`.

## See Also

- [`Series::scatter_reduce`](scatter_reduce.md): out-of-place sibling.
- [`Series::scatter_add_`](scatter_add_.md): in-place additive shorthand.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scatter_reduce.html">
    <span class="label">Previous</span>
    <span class="title">scatter_reduce</span>
  </a>
  <a class="next-link" href="scatter_reduce_amax.html">
    <span class="label">Next</span>
    <span class="title">scatter_reduce_amax</span>
  </a>
</div>
