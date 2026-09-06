<!-- pluss-hand-authored -->

# Series::is_contiguous

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_contiguous() const
```

## What It Does

Returns `true` when the `Series`' [`strides`](stride.md) describe the
**standard packed layout** for its current shape — the layout a freshly
created tensor has, where the last axis has stride `1` and each earlier axis
steps over one complete block of the axes inside it.

It is a layout question, not a correctness or ownership question. A
non-contiguous `Series` — a transpose, a broadcast, a stepped slice — still
shares valid storage with its base and still produces correct results from
almost every operation. `is_contiguous()` only tells you whether the values
are laid out in memory in the same order you would visit them logically.

Why it matters: a handful of operations (and any code that reaches for
[`raw_data()`](../../series_api_reference) directly, or hands the buffer to an
external library) need that packed order and will call
[`contiguous()`](contiguous.md) internally, which **copies** when the input is
not already contiguous. `is_contiguous()` lets you see that cost coming, and
`x.contiguous()` is a no-op (returns the same storage) when `x` already is —
so calling it defensively is cheap, but knowing whether it will copy is
sometimes worth checking first.

## Parameters

None.

## Returns

`bool`
    `true` iff the current strides are the packed strides for the current
    shape. A scalar and an empty `Series` are trivially contiguous.

## Gradient

Not applicable — `is_contiguous()` returns a plain `bool`.

## Examples

Example 1. The common layouts, contiguous or not.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto row = pluss::make_series({1.0, 2.0, 3.0});
auto v = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
std::cout << std::boolalpha
          << "fresh matrix:       " << m.is_contiguous() << "\n"
          << "reshape({3,2}):     " << m.reshape({3, 2}).is_contiguous() << "\n"
          << "transpose(0,1):     " << m.transpose(0, 1).is_contiguous() << "\n"
          << "transpose.contig(): " << m.transpose(0, 1).contiguous().is_contiguous() << "\n"
          << "expand({4,3}):      " << row.unsqueeze(0).expand({4, 3}).is_contiguous() << "\n"
          << "slice step 2:       " << v.slice(0, 0, 6, 2).is_contiguous() << "\n"
          << "contiguous slice:   " << v.slice(0, 1, 4).is_contiguous() << "\n"
          << "scalar:             " << pluss::make_series(7.0).is_contiguous() << "\n"
          << "empty zeros({0}):   " << pluss::series::zeros({0}).is_contiguous() << "\n";
```

### Output

```text
fresh matrix:       true
reshape({3,2}):     true
transpose(0,1):     false
transpose.contig(): true
expand({4,3}):      false
slice step 2:       false
contiguous slice:   true
scalar:             true
empty zeros({0}):   true
```

`reshape` of a contiguous tensor stays contiguous (it only relabels the same
packed run). `transpose` reorders the strides and breaks it. `expand`
introduces a stride-`0` axis. A **stepped** slice (`step = 2`) skips storage
cells; a plain contiguous slice along the first axis does not.

Example 2. `contiguous()` on an already-contiguous `Series` returns the same
storage — no copy.

```cpp
auto m = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto c = m.contiguous();
std::cout << "same storage: " << std::boolalpha << m.is_set_to(c) << "\n";
```

### Output

```text
same storage: true
```

[`is_set_to`](is_set_to.md) being `true` here confirms `c` and `m` describe
the identical storage and view — `contiguous()` did nothing but hand `m`
back.

Example 3. A non-contiguous view is still fully usable — `is_contiguous()`
is not a validity check.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto t = m.transpose(0, 1);
std::cout << "t.is_contiguous()=" << std::boolalpha << t.is_contiguous() << "\n";
std::cout << "t.sum()=";
pluss::series::print(std::cout, t.sum());
```

### Output

```text
t.is_contiguous()=false
t.sum()=Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=21.0000
```

The transposed view sums to `21` (all six values) exactly as its base would.
Reach for [`contiguous()`](contiguous.md) only when a specific downstream
step demands packed storage — not because a stride differs from the default.

## Gotchas And Failure Modes

- Non-contiguous is not invalid. Transposed, broadcast, and stepped-slice
  views work with nearly every operation.
- `reshape` requires a contiguous input (or a view that can be reshaped
  without copy); on a non-contiguous `Series` it falls back to a copy. If
  the copy matters, check `is_contiguous()` first.
- A broadcast (`expand`) view is non-contiguous *and* has stride-`0` axes —
  see [`stride`](stride.md). Materializing it with `contiguous()` allocates
  the full expanded size.
- A scalar and every empty `Series` report `true` — there is no non-trivial
  layout for them to violate.
- `contiguous()` is a no-op when the input already is, so calling it
  defensively costs nothing in that case; the check only matters when you
  want to *avoid* the copy on the non-contiguous path.

## Summary

`is_contiguous()` reports whether the strides are the standard packed layout
for the current shape:

- `true` for a fresh tensor, a `reshape` of one, a plain (non-stepped)
  leading-axis slice, a scalar, and any empty `Series`
- `false` for a transpose, a broadcast/`expand`, and a stepped slice
- a layout fact, not a validity or ownership fact
- pair with [`contiguous()`](contiguous.md), which copies only when the
  input is not already contiguous

## See Also

- [`Series::contiguous`](contiguous.md): materialize a packed copy (a no-op
  when already contiguous).
- [`Series::stride`](stride.md) / [`Series::sizes`](sizes.md): the layout
  `is_contiguous()` is comparing against the packed ideal.
- [`Series::is_set_to`](is_set_to.md): exact storage-and-geometry identity —
  how Example 2 confirms `contiguous()` did not copy.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_complex.html">
    <span class="label">Previous</span>
    <span class="title">is_complex</span>
  </a>
  <a class="next-link" href="is_cpu.html">
    <span class="label">Next</span>
    <span class="title">is_cpu</span>
  </a>
</div>
