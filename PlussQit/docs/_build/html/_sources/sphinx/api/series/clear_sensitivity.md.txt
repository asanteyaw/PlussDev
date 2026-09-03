<!-- pluss-hand-authored -->

# Series::clear_sensitivity

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::clear_sensitivity() const -> void
```

## What It Does

`clear_sensitivity()` removes the currently stored reverse-mode sensitivity from
this `Series`.

That is a narrower operation than many readers first assume.

It does:

- forget the stored sensitivity value, if one exists

It does not:

- disable [`tracks_slope()`](tracks_slope.md)
- alter the actual numeric data in the `Series`
- remove the ability to build future reverse-mode graphs from this value

So the right mental model is "clear the stored sensitivity state," not
"detach this series from reverse-mode entirely."

## Parameters

None.

This is a state-reset operation on `self`.

## Returns

`void`
    The method mutates stored autograd state and does not return a value.

## Mathematical Meaning

If a reverse pass stored `dL/dx` on a tracked leaf `x`, then
`clear_sensitivity()` deletes that stored public result.

It does not change what `dL/dx` was for the already-run computation. It only
changes what is still stored on the object afterward.

That is why this method is about bookkeeping, not differentiation. It does not
recompute anything and does not change the forward function.

## When To Use It

`clear_sensitivity()` is most useful in three situations:

- you want to inspect one reverse pass, then discard it before the next one
- you are reusing a tracked leaf across separate computations and want a clean
  stored sensitivity buffer
- you want `has_sensitivity()` to become `false` again without disabling
  tracking

That last point is important. Clearing stored sensitivity and turning tracking
off are separate controls.

## Examples

Example 1. After a reverse pass, `clear_sensitivity()` removes the stored
sensitivity and `has_sensitivity()` becomes `false` again.

```cpp
auto life = pluss::make_series({2.0, 4.0, 8.0}, pluss::cfg::tracks_slope());

std::cout << "before=" << (life.has_sensitivity() ? "true" : "false") << "\n";
life.log().sum().reverse();
std::cout << "after_reverse=" << (life.has_sensitivity() ? "true" : "false") << "\n";
life.clear_sensitivity();
std::cout << "after_clear=" << (life.has_sensitivity() ? "true" : "false") << "\n";
```

The reverse pass computes a real stored sensitivity for the scalar objective

```text
L = log(2) + log(4) + log(8)
```

and `clear_sensitivity()` then removes that stored result.

### Output

```text
before=false
after_reverse=true
after_clear=false
```

Example 2. Clearing between passes lets the next reverse pass start from an
empty stored sensitivity buffer.

```cpp
auto reuse = pluss::make_series({1.0, 2.0, 3.0});
reuse.tracks_slope_(true);

reuse.mul(2.0).sum().reverse();
std::cout << "first_dL_dx=" << reuse.sensitivity() << "\n";

reuse.clear_sensitivity();
reuse.mul(5.0).sum().reverse();
std::cout << "second_dL_dx=" << reuse.sensitivity() << "\n";
```

The first scalar objective is

```text
L1 = 2x1 + 2x2 + 2x3
```

so the stored sensitivity is `[2, 2, 2]`.

After clearing, the second scalar objective is

```text
L2 = 5x1 + 5x2 + 5x3
```

so the new stored sensitivity is `[5, 5, 5]`.

### Output

```text
first_dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 2, 2]
second_dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5, 5, 5]
```

The second pass is not added on top of the first one because the stored
sensitivity was cleared before it ran.

Example 3. Clearing stored sensitivity does not disable tracking.

```cpp
auto keep_track = pluss::make_series({9.0, 10.0});
keep_track.tracks_slope_(true);
keep_track.mul(4.0).sum().reverse();
keep_track.clear_sensitivity();

std::cout << "tracks=" << (keep_track.tracks_slope() ? "true" : "false") << "\n";
std::cout << "has_dL_dx=" << (keep_track.has_sensitivity() ? "true" : "false") << "\n";
```

The leaf is still configured to participate in future reverse passes even
though its current stored sensitivity was discarded.

### Output

```text
tracks=true
has_dL_dx=false
```

Example 4. Calling `clear_sensitivity()` on a series that does not currently
store sensitivity is a harmless no-op.

```cpp
auto noop = pluss::make_series({7.0, 8.0});
noop.clear_sensitivity();
std::cout << "has_dL_dx=" << (noop.has_sensitivity() ? "true" : "false") << "\n";
```

### Output

```text
has_dL_dx=false
```

## Gotchas And Failure Modes

- `clear_sensitivity()` does not turn tracking off. Use
  [`tracks_slope_`](tracks_slope_.md) for that.
- Clearing stored sensitivity does not change the forward values.
- This method does not recompute derivatives. It only removes the stored
  public result of an earlier reverse pass.

## Summary

Use `clear_sensitivity()` when you want to forget the current stored
sensitivity while keeping the `Series` itself and its tracking configuration
intact.

## See Also

- [`Series::has_sensitivity`](has_sensitivity.md): test whether a stored
  sensitivity currently exists.
- [`Series::sensitivity`](sensitivity.md): read the stored value itself.
- [`Series::tracks_slope_`](tracks_slope_.md): enable or disable tracking.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clamp_min.html">
    <span class="label">Previous</span>
    <span class="title">clamp_min</span>
  </a>
  <a class="next-link" href="clone.html">
    <span class="label">Next</span>
    <span class="title">clone</span>
  </a>
</div>
