<!-- pluss-hand-authored -->

# Series::stat

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::stat -> SeriesStatAccessor
Series::stat() -> SeriesStatProxy
Series::stat(const Series& other) -> SeriesStatProxy
Series::stat(std::initializer_list<Series> others) -> SeriesStatProxy
Series::stat(std::span<const Series> others) -> SeriesStatProxy
```

## What It Does

`stat` is the namespace-like accessor for statistical work on a `Series`.

That matters because `stat` itself is not a reduction or transform. It is an
entrypoint into a family of related operations:

- scalar or axis-wise summaries like `var`, `std`, `nanvar`, `nanstd`
- shape-preserving normalization transforms like `center` and `standardize`
- higher-moment summaries like `skewness` and `kurtosis`
- a wider peer-bound surface such as pairwise distances, documented later

So the right mental model is not "call `stat` to do statistics." The right
model is:

- `stat` groups the statistical vocabulary
- the terminal method does the actual work

That grouping is valuable because it gives one predictable place to look when
the task is "statistics on this series" rather than "arbitrary elementwise
math."

## Overload Map

**`x.stat.method(...)`**

The shortest and most common spelling. This is the ordinary one-series accessor
path.

**`x.stat().method(...)`**

The explicit proxy spelling. It belongs to the same family and is mostly useful
when you want to emphasize the callable accessor step itself.

**`x.stat(other)`**, **`x.stat({a, b, c})`**, **`x.stat(span)`**

Peer-binding forms for methods whose semantics depend on another series or a
peer set. Those overloads are part of the public surface even though the
current handwritten batch focuses first on the one-series descriptive subset.

## Parameters

The dot-style accessor form `x.stat` takes no parameters by itself.

The callable proxy forms accept optional peer `Series` inputs, but that does
not mean statistical tuning happens on `stat` itself. The real statistical
parameters live on the terminal method you call next:

- `dim`
- `keepdim`
- `unbiased`
- NaN-handling choices
- and so on

So the teaching rule is: `stat` chooses the family; the terminal method chooses
the computation.

## Returns

`SeriesStatAccessor`
    A lightweight accessor bound to the current series.

`SeriesStatProxy`
    A temporary proxy used when you write `x.stat()` or bind peer series with
    `x.stat(other)` before calling a terminal method.

## Verified Examples

Example 1. Use the accessor for axis-wise variance without leaving the original
series namespace.

```cpp
auto values = pluss::make_series({1, 2, 3, 4, 5, 6}, {2, 3});

std::cout << values.stat.var(1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66666667, 0.66666667]
```

This is the most important first mental model: `stat` does not copy data or
change semantics by itself. It simply gives you a grouped spelling for the same
underlying statistical family.

Example 2. The explicit callable proxy form lands on the same family.

```cpp
std::cout << values.stat().var(1, false);
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.66666667, 0.66666667]
```

So `x.stat.var(...)` and `x.stat().var(...)` are not two different algorithms.
They are two routes into the same accessor family.

Example 3. Shape-preserving transforms live on the same accessor.

```cpp
std::cout << values.stat.standardize(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.2247449, 0, 1.2247449],
  [-1.2247449, 0, 1.2247449]]
```

So the accessor is not only for reductions. It also groups transforms that are
statistical in intent but preserve the original geometry.

Example 4. The explicit proxy form works for transforms too.

```cpp
std::cout << values.stat().center(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1, 0, 1],
  [-1, 0, 1]]
```

That matters pedagogically because it proves the callable accessor is not just
for peer-bound future methods. It is a first-class route into the same
descriptive family.

Example 5. The same is true for `standardize()`.

```cpp
std::cout << values.stat().standardize(1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[-1.2247449, 0, 1.2247449],
  [-1.2247449, 0, 1.2247449]]
```

Example 6. Higher moments are reached through the same entrypoint.

```cpp
auto lopsided = pluss::make_series({1, 1, 4, 2, 2, 8}, {2, 3});

std::cout << lopsided.stat.skewness();
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[1.2247449]
```

That one-page jump from `var` to `standardize` to `skewness` is the real reason
the accessor exists: it gives one stable place to look when the task is
"statistics on this series," not "arbitrary elementwise math."

## Notes

- `stat` itself does not compute anything until you call one of its methods.
- Dot-style convenience (`x.stat.var(...)`) and explicit proxy style
  (`x.stat().var(...)`) belong to the same family.
- The peer-bound callable forms are part of the public surface, but this
  handwritten batch focuses first on the one-series descriptive subset.
- Each terminal accessor method has its own dedicated page and should be read
  for the real algorithm-specific parameter rules.

## See Also

- [SeriesStatAccessor API Reference](../../series_stat_accessor_api_reference):
  dedicated landing page for the current handwritten accessor subset.
- [`Series::mean`](mean.md): direct reduction page for the arithmetic mean.
- [`Series::sum`](sum.md): ordinary numeric reduction outside the accessor
  grouping.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="argmin.html">
    <span class="label">Previous</span>
    <span class="title">argmin</span>
  </a>
  <a class="next-link" href="../../series_stat_accessor_api_reference.html">
    <span class="label">Next</span>
    <span class="title">SeriesStatAccessor API Reference</span>
  </a>
</div>
