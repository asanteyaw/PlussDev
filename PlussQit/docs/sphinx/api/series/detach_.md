<!-- pluss-hand-authored -->

# Series::detach_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::detach_() const -> Series&
```

## What It Does

`detach_()` detaches the current `Series` handle in place.

This is the mutating sibling of [`detach()`](detach.md). Instead of returning a
separate detached value and leaving the original handle untouched, it changes
the receiver so that future work from this handle no longer follows the earlier
reverse-mode path.

## Parameters

None.

## Returns

`Series&`
    The current `Series`, after detaching its reverse-mode state in place.

## Reverse-Mode Meaning

After `detach_()` on the current handle:

- the receiver is no longer tracked
- later reverse-mode built from that receiver no longer propagates through the
  earlier path

The important mental model is "this handle becomes a new autodiff boundary."

That means the numeric value remains, but the reverse-mode history attached to
this handle is cut.

## Examples

Example 1. `detach_()` clears tracking on the receiver itself.

```cpp
auto x = pluss::make_series({6.0, 7.0});
x.tracks_slope_(true);

std::cout << "before=" << (x.tracks_slope() ? "true" : "false") << "\n";
x.detach_();
std::cout << "after=" << (x.tracks_slope() ? "true" : "false") << "\n";
```

### Output

```text
before=true
after=false
```

Example 2. Detaching an intermediate in place blocks the already-built
upstream path from receiving sensitivity.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
x.tracks_slope_(true);
auto y = x.mul(2.0);
y.detach_();
y.sum().reverse();

std::cout << "x_has_dL_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
```

Before the detach, the chain would have been

```text
y = 2x
L = sum(y)
```

which ordinarily gives `dL/dx = [2, 2, 2]`.

After `y.detach_()`, the handle `y` becomes a new boundary, so the later
reverse pass no longer propagates through the earlier `x -> y` path.

### Output

```text
x_has_dL_dx=false
```

Example 3. After in-place detaching, the same handle can be turned into a new
tracked leaf.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0}, pluss::cfg::tracks_slope());
auto y = x.mul(2.0);
y.detach_();
y.tracks_slope_(true);
y.sum().reverse();

std::cout << "x_has_dL_dx=" << (x.has_sensitivity() ? "true" : "false") << "\n";
std::cout << "y_dL_dy=" << y.sensitivity() << "\n";
```

Now the scalar objective is differentiated with respect to the post-detach
handle `y` itself:

```text
L = y1 + y2 + y3
```

so the new leaf stores `[1, 1, 1]`, while the original upstream source `x`
still receives nothing from this new reverse path.

### Output

```text
x_has_dL_dx=false
y_dL_dy=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 1]
```

## Gotchas And Failure Modes

- `detach_()` mutates the receiver. If you need the original tracked handle to
  remain unchanged, use [`detach()`](detach.md) instead.
- This method is about reverse-mode state, not numeric value transformation.
- After detaching in place, later reverse-mode from that handle no longer
  populates upstream sensitivity through the earlier graph.

## Summary

Use `detach_()` when the current handle itself should stop participating in the
existing reverse-mode path and become a fresh boundary for whatever comes next.

## See Also

- [`Series::detach`](detach.md): non-mutating detach that returns a detached
  value.
- [`Series::tracks_slope_`](tracks_slope_.md): re-enable tracking later if this
  handle should become a new tracked leaf.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="detach.html">
    <span class="label">Previous</span>
    <span class="title">detach</span>
  </a>
  <a class="next-link" href="diag.html">
    <span class="label">Next</span>
    <span class="title">diag</span>
  </a>
</div>
