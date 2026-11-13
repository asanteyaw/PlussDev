<!-- pluss-hand-authored -->

# Series::contiguous

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::contiguous() const
```

## What It Does

`contiguous()` returns a contiguous-layout `Series` with the same visible
values as the receiver.

This method is about layout, not value transformation. It does not change:

- shape
- dtype
- platform
- visible element order

What it can change is storage arrangement. That matters most when `self` is a
view such as a transpose, slice, or other non-contiguous layout.

One public detail is especially important: if `self` is already contiguous, the
call does not need to change the visible result at all. If you need a
guaranteed new buffer, `contiguous()` is not the method that states that
contract clearly; [`clone()`](clone.md) is.

## Parameters

None.

## Returns

`Series`
    A contiguous `Series`.

If the receiver was already contiguous, the visible result is unchanged. If the
receiver was not contiguous, the result is materialized into a new contiguous
buffer.

## Gradient

`contiguous()` preserves reverse-mode connectivity on the current verified
surface.

When the method has to materialize a new buffer, sensitivity still routes back
to the original source. When the input is already contiguous, there is no
layout change to reason about in the first place.

## Examples

Example 1. Calling `contiguous()` on a non-contiguous view materializes a new
contiguous result.

```cpp
auto contig_view = pluss::make_series({
    {10.0, 20.0},
    {30.0, 40.0},
    {50.0, 60.0},
}).transpose(0, 1);

auto contig_copy = contig_view.contiguous();
contig_copy.add_(7.0);

std::cout << "view=" << contig_view << "\n";
std::cout << "copy=" << contig_copy << "\n";
std::cout << "before=" << (contig_view.is_contiguous() ? "true" : "false") << "\n";
std::cout << "after=" << (contig_copy.is_contiguous() ? "true" : "false") << "\n";
```

### Output

```text
view=pluss::Series(shape=[2,3], strides=[1,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[10, 30, 50],
  [20, 40, 60]]
copy=pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[17, 37, 57],
  [27, 47, 67]]
before=false
after=true
```

The view and the contiguous copy diverge after mutation, which is exactly what
you want from a materialized layout normalization step.

Example 2. Calling `contiguous()` on an already contiguous series is a no-op
in terms of visible layout: it stays contiguous and its values stay the same.

```cpp
auto already = pluss::make_series({3.0, 6.0, 9.0});
auto still_contiguous = already.contiguous();

std::cout << "before=" << (already.is_contiguous() ? "true" : "false") << "\n";
std::cout << "after=" << (still_contiguous.is_contiguous() ? "true" : "false") << "\n";
std::cout << still_contiguous << "\n";
```

### Output

```text
before=true
after=true
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[3, 6, 9]
```

Example 3. Reverse-mode sensitivity passes through `contiguous()` back to the
original source.

```cpp
auto contig_grad = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});
contig_grad.tracks_slope_(true);
contig_grad.transpose(0, 1).contiguous().sum().reverse();

std::cout << contig_grad.sensitivity();
```

### Output

```text
pluss::Series(shape=[2,3], strides=[1,2], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[1, 1, 1],
  [1, 1, 1]]
```

## Gotchas And Failure Modes

- `contiguous()` is a layout method, not an ownership promise. Use
  [`clone()`](clone.md) when you need a guaranteed standalone copy.
- The method preserves visible value order, not the original backing-storage
  arrangement of some earlier source.
- This is a layout operation only. Use [`to()`](to.md) for representation
  changes and [`clone()`](clone.md) when you want an unconditional copy.

## Summary

Use `contiguous()` when layout is the problem.

- non-contiguous inputs materialize into a standalone contiguous buffer
- already-contiguous inputs keep the same visible layout and values
- reverse-mode remains connected on the current verified surface

## See Also

- [`Series::clone`](clone.md): unconditional standalone copy.
- [`Series::copy_`](copy_.md): overwrite an existing destination in place.
- [`Series::to`](to.md): representation-changing conversion that also
  materializes contiguous output when needed.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="conj_physical.html">
    <span class="label">Previous</span>
    <span class="title">conj_physical</span>
  </a>
  <a class="next-link" href="copy_.html">
    <span class="label">Next</span>
    <span class="title">copy_</span>
  </a>
</div>
