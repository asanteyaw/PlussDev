<!-- pluss-hand-authored -->

# Series::detach

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::detach() const
```

## What It Does

`detach()` returns a new `Series` value with the same visible data but without
the current reverse-mode tracking lineage.

This is the read-side, non-mutating form of detaching.

Use it when you want:

- the current numeric values
- no current reverse-mode connection back through how those values were
  produced

That is the real contract. The important promise is about reverse-mode state,
not about forcing one specific storage strategy.

## Parameters

None.

## Returns

`Series`
    A detached `Series` value.

On the current verified public surface, the detached result starts untracked.

## Reverse-Mode Meaning

Detaching creates a new autodiff boundary.

Before detaching, later reverse-mode can treat the current value as depending
on earlier tracked sources.

After detaching, later reverse-mode treats the detached value as a fresh value
with no public reverse-mode path back through the pre-detach computation.

In mathematical terms, if an earlier tracked source `x` produced some
intermediate value and you then call `detach()`, later objectives built from
the detached result are no longer differentiated with respect to that earlier
path.

## Examples

Example 1. `detach()` preserves the visible values.

```cpp
auto base = pluss::make_series({1.0, 2.0, 3.0});
auto detached = base.detach();

std::cout << "base=" << base << "\n";
std::cout << "detached=" << detached << "\n";
```

### Output

```text
base=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
detached=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

Example 2. On the current verified surface, later in-place mutation of the
detached result does not rewrite the original series in this example.

```cpp
auto base = pluss::make_series({1.0, 2.0, 3.0});
auto detached = base.detach();
detached.add_(10.0);

std::cout << "base=" << base << "\n";
std::cout << "detached=" << detached << "\n";
```

### Output

```text
base=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
detached=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[11, 12, 13]
```

This is worth documenting carefully: the public contract you should rely on is
the detach of reverse-mode state. Storage-sharing details are not the point of
the method.

Example 3. Reverse-mode built from the detached result no longer populates
sensitivity on the original tracked source.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0}, pluss::cfg::tracks_slope());

auto y = x.detach().mul(3.0);
y.sum().reverse();

std::cout << "x_has_dL_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
```

The scalar objective is

```text
L = sum(3 * detach(x))
```

Numerically, the detached value contains the same entries as `x`, but for
reverse-mode purposes it is now a fresh boundary. So this objective is not
propagated back through the original tracked `x`.

### Output

```text
x_has_dL_dx=false
```

Example 4. The detached result starts with tracking disabled even if the
original source was tracked.

```cpp
auto source = pluss::make_series({4.0, 5.0});
source.tracks_slope_(true);
auto detached = source.detach();

std::cout << "source_tracks=" << (source.tracks_slope() ? "true" : "false") << "\n";
std::cout << "detached_tracks=" << (detached.tracks_slope() ? "true" : "false") << "\n";
```

### Output

```text
source_tracks=true
detached_tracks=false
```

Example 5. A detached result can become a new tracked leaf if you enable
tracking on the detached value itself.

```cpp
auto source = pluss::make_series({1.0, 2.0, 3.0}, pluss::cfg::tracks_slope());
auto detached = source.detach();
detached.tracks_slope_(true);

detached.mul(3.0).sum().reverse();

std::cout << "source_has_dL_dsource=" << (source.has_sensitivity() ? "true" : "false") << "\n";
std::cout << "detached_dL_ddetached=" << detached.sensitivity() << "\n";
```

The new scalar objective is differentiated with respect to the detached leaf,
not with respect to the original source:

```text
L = 3d1 + 3d2 + 3d3
```

so the detached leaf stores `[3, 3, 3]`, while the original source remains
outside the new reverse path.

### Output

```text
source_has_dL_dsource=false
detached_dL_ddetached=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3, 3, 3]
```

## Gotchas And Failure Modes

- `detach()` is about severing reverse-mode lineage, not about changing the
  values.
- The detached result starts untracked.
- If you want to detach the current handle in place instead of creating a new
  value, use [`detach_`](detach_.md).
- If you later re-enable tracking on the detached result, it becomes a new leaf
  with a new reverse history, not a continuation of the old one.

## Summary

Use `detach()` when you want the current numeric value as a fresh autodiff
boundary: same visible data, no current upstream reverse-mode connection.

## See Also

- [`Series::detach_`](detach_.md): in-place detach on the current handle.
- [`Series::tracks_slope_`](tracks_slope_.md): turn the detached result into a
  new tracked leaf if needed.
- [`Series::reverse`](reverse.md): the downstream reverse entrypoint that
  detached values stop feeding into upstream history.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="deg2rad.html">
    <span class="label">Previous</span>
    <span class="title">deg2rad</span>
  </a>
  <a class="next-link" href="detach_.html">
    <span class="label">Next</span>
    <span class="title">detach_</span>
  </a>
</div>
