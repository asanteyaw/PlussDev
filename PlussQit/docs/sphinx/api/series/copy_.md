<!-- pluss-hand-authored -->

# Series::copy_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series& Series::copy_(const Series& src) const
Series& Series::copy_(const Series& src, bool non_blocking) const
```

## What It Does

Overwrites the current series with `src` and returns the same handle.

`copy_()` is a storage mutation, not a value-producing expression. That is the
important mental model. After the call:

- the destination still has the same identity
- its storage now contains `src`'s values
- dtype/platform conversion and broadcasting happen as needed to make the write
  legal

This is why `copy_()` shows up in parameter initialization code so often. A
module can allocate the parameter object once, register it once, and then fill
its storage with real values without replacing the parameter handle itself.

## Parameters

**src** (`const Series&`)

The source value to write into the current series.

`src` may differ from the destination in:

- shape, as long as it broadcasts to the destination shape
- dtype, in which case it is converted
- platform, in which case the data is transferred

**non_blocking** (`bool`)

Transfer hint for cross-platform copies. It affects scheduling, not the
numerical result.

## Returns

`Series&`
    The same destination series, after mutation.

## Gradient

`copy_()` is one of the few methods where the backward story depends heavily on
what the destination already was.

If the destination is a freshly allocated tracked leaf that has not yet been
used in a graph, `copy_()` is the normal and safe way to initialize it. Later
gradients still accumulate on that leaf exactly as expected.

If the destination is instead a tracked view into an existing tracked value,
then the overwritten region stops attributing its future gradient to the old
producer and starts attributing it to `src`. That is the correct semantics for
"these positions now came from somewhere else."

## Examples

Example 1. `copy_()` mutates and returns the same destination object.

```cpp
auto dst = pluss::make_series({0.0, 0.0, 0.0}, {3});
auto src = pluss::make_series({1.0, 2.0, 3.0}, {3});

auto& same = dst.copy_(src);
std::cout << "dst=" << dst << "\n";
std::cout << "same_object=" << (&same == &dst ? "true" : "false") << "\n";
```

### Output

```text
dst=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
same_object=true
```

Nothing new was allocated as the result of the method itself. The returned
reference is just `dst`.

Example 2. Initializing a tracked leaf parameter with `copy_()` does not poison
its later gradient.

```cpp
pluss::Series w = pluss::Series::empty({5, 3}, pluss::ScalarType::Float64, pluss::pCPU);
w.tracks_slope_(true);
w.copy_(pluss::make_series({
    {1.0, 1.0, 1.0},
    {2.0, 2.0, 2.0},
    {3.0, 3.0, 3.0},
    {4.0, 4.0, 4.0},
    {5.0, 5.0, 5.0},
}));
w.clear();

auto picked_rows = pluss::make_series({0, 2, 4}, {3});
auto picked = w.index_select(0, picked_rows);
picked.sum().reverse();
std::cout << "picked=" << picked << "\n";
std::cout << "dw=" << w.sensitivity() << "\n";
```

### Output

```text
picked=pluss::Series(shape=[3,3], strides=[3,1], dtype=float64, platform=cpu, numel=9, contiguous=true)
data=[[1, 1, 1],
  [3, 3, 3],
  [5, 5, 5]]
dw=pluss::Series(shape=[5,3], strides=[3,1], dtype=float64, platform=cpu, numel=15, contiguous=true)
data=[[1, 1, 1],
  [0, 0, 0],
  [1, 1, 1],
  [0, 0, 0],
  [1, 1, 1]]
```

Rows `0`, `2`, and `4` were looked up later, so those rows receive gradient.
Rows `1` and `3` do not. The one-time `copy_()` initialization did not erase or
misroute any of that later sensitivity.

Example 3. Copying into a tracked view redirects the overwritten region's
gradient to the replacement source.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4});
x.tracks_slope_(true);

auto replacement = pluss::make_series({10.0, 20.0}, {2});
replacement.tracks_slope_(true);

auto middle = x.slice(0, 1, 3, 1);
middle.copy_(replacement);

x.sum().reverse();
std::cout << "x=" << x << "\n";
std::cout << "dx=" << x.sensitivity() << "\n";
std::cout << "dsrc=" << replacement.sensitivity() << "\n";
```

### Output

```text
x=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 10, 20, 4]
dx=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 0, 0, 1]
dsrc=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

Positions `1` and `2` no longer attribute their future value to `x`'s old
contents. Those positions now came from `replacement`, so the gradient follows
that fact.

## Notes

- `copy_()` is a mutation primitive. Reach for it when handle identity matters.
- It is the right tool for filling pre-existing parameter storage.
- It is not just "assignment with a different name"; its graph behavior matters
  in tracked view scenarios.

## See Also

- [`Series::index_select`](index_select.md): the lookup pattern used in Example
  2.
- [`Series::gather`](gather.md): the more general read-side gather primitive.
- [`Series::slice`](slice.md): how to create the kind of view used in Example 3.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contiguous.html">
    <span class="label">Previous</span>
    <span class="title">contiguous</span>
  </a>
  <a class="next-link" href="copysign.html">
    <span class="label">Next</span>
    <span class="title">copysign</span>
  </a>
</div>
