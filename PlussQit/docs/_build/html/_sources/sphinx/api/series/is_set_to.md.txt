<!-- pluss-hand-authored -->

# Series::is_set_to

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
bool Series::is_set_to(const Series& other) const noexcept
```

## What It Does

Returns `true` only when `self` and `other` describe the **exact same view
of the exact same storage** — same underlying buffer pointer, same storage
offset, same dtype, same platform, same shape, **and** same strides.

It is the strictest of the identity/equality checks on `Series`, and much
stronger than "these have equal values":

- **Copying a `Series` handle** (`auto b = a;`) keeps the identity — both
  refer to the same storage and geometry, so `a.is_set_to(b)` is `true`.
- **[`clone()`](clone.md)** allocates a fresh buffer, so a clone can be
  value-[`equal`](equal.md) without being set to the same storage.
- **A transpose or a slice** shares the base allocation but describes a
  *different* view — different strides, or a different offset and shape — so
  `is_set_to()` is `false` even though the storage is shared.

This is the check for "is this literally the same tensor object under two
names" — useful when reasoning about aliasing, verifying that a supposedly
in-place operation did not quietly reallocate, or confirming
[`contiguous()`](contiguous.md) returned its input untouched.

`is_set_to()` does not look at current values. Two aliases stay set to the
same view across an in-place (`_`-suffixed) mutation: the values are written
back into the shared storage, so both handles see the change and the
identity holds. The only in-place ops that break the identity are the ones
that *have* to — a comparison `_` such as `gt_` that produces a `Bool`
result, or a shape-changing `squeeze_` — because the result no longer fits
the original buffer.

Two undefined `Series` are set to each other; a defined/undefined pair is
not.

## Parameters

**other** (`const Series&`): the `Series` to compare identity with.

## Returns

`bool`
    `true` iff storage pointer, storage offset, dtype, platform, shape, and
    strides all match.

## Gradient

Not applicable — `is_set_to()` returns a plain `bool`.

## Examples

Example 1. Handle copy vs. transpose vs. slice vs. clone.

```cpp
auto base = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
auto alias = base;                 // handle copy
auto view = base.transpose(0, 1);  // shares storage, different geometry
auto sl = base.slice(0, 0, 1);     // shares storage, different offset/shape
auto copy = base.clone();          // independent storage
std::cout << std::boolalpha
          << "set_to alias:     " << base.is_set_to(alias) << "\n"
          << "set_to transpose: " << base.is_set_to(view) << "\n"
          << "set_to slice:     " << base.is_set_to(sl) << "\n"
          << "set_to clone:     " << base.is_set_to(copy) << "\n"
          << "equal  clone:     " << base.equal(copy) << "\n";
```

### Output

```text
set_to alias:     true
set_to transpose: false
set_to slice:     false
set_to clone:     false
equal  clone:     true
```

Only the handle copy is *the same tensor*. The transpose and the slice reuse
`base`'s buffer but present it differently. The clone is a separate buffer
that happens to hold the same numbers — value-`equal`, not `set_to`.

Example 2. `contiguous()` on an already-contiguous `Series` returns its
input — `is_set_to()` confirms no copy happened.

```cpp
auto m = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
auto c = m.contiguous();
std::cout << "contiguous() copied? " << std::boolalpha << !m.is_set_to(c) << "\n";
```

### Output

```text
contiguous() copied? false
```

On a *non*-contiguous input `contiguous()` would allocate, and
`m.is_set_to(c)` would be `false`.

Example 3. A value-preserving in-place write keeps two aliases set to the
same view — the mutation is visible through both handles.

```cpp
auto base = pluss::make_series({1.0, 2.0, 3.0});
auto alias = base;
alias.add_(10.0);
std::cout << std::boolalpha << "still set_to: " << base.is_set_to(alias) << "\n";
pluss::series::print(std::cout, base);
```

### Output

```text
still set_to: true
Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[11.0000, 12.0000, 13.0000]
```

`add_` wrote through the shared buffer, so `base` sees `[11, 12, 13]` and the
identity holds. The same is true of every in-place op whose result keeps the
tensor's shape and dtype — `mul_`, `neg_`, `sin_`, `relu_`, `fill_`,
`clamp_min_`, the `index_*_` / `scatter_` / `masked_fill_` family, and so on.

Example 4. An in-place op that must change the dtype or shape has no choice
but to rebind — and `is_set_to()` reports that.

```cpp
auto base = pluss::make_series({1.0, 5.0, 3.0});
auto alias = base;
alias.gt_(pluss::make_series({2.0, 2.0, 2.0}));   // result is Bool
std::cout << std::boolalpha
          << "still set_to: " << base.is_set_to(alias) << "\n";
std::cout << "base dtype:  " << base.dtype() << "\n";
std::cout << "alias dtype: " << alias.dtype() << "\n";
```

### Output

```text
still set_to: false
base dtype:  float64
alias dtype: bool
```

`gt_` turns `alias` into a `Bool` tensor, which cannot occupy the original
`float64` buffer, so `alias` is rebound and `base` keeps its own values. A
shape-changing `squeeze_` breaks the identity the same way. When it matters,
`is_set_to()` before and after tells you whether a rebind happened.

## Choosing The Identity Test

| Question | Use |
|---|---|
| Same shape? | [`is_same_size`](is_same_size.md) |
| Same shape **and** values? | [`equal`](equal.md) |
| Literally the same tensor (storage + geometry)? | `is_set_to` |

## Gotchas And Failure Modes

- `is_set_to()` is not value equality. A [`clone()`](clone.md) is
  value-[`equal`](equal.md) but never `is_set_to` its source.
- Sharing the base allocation is not enough — a transpose, a slice, a
  reshape-that-copies, or a dtype view all describe *different* geometry and
  return `false`.
- A value-preserving in-place `_` op (one whose result keeps the tensor's
  shape and dtype) writes through the shared storage, so aliases stay
  `is_set_to`. Only an op that changes dtype (a comparison `_` → `Bool`) or
  shape (`squeeze_`) rebinds the handle.
- Two undefined `Series` are set to each other; a defined/undefined pair is
  not.
- `noexcept` and cheap.

## Summary

`is_set_to(other)` tests exact storage-and-view identity:

- `true` only for a handle copy (or another reference to the identical view)
- `false` for a transpose, slice, or [`clone()`](clone.md), even when
  storage or values are shared
- ignores current values; survives any in-place `_` write that keeps the
  tensor's shape and dtype, and is broken only by one that changes them
- the strongest of [`is_same_size`](is_same_size.md) / [`equal`](equal.md) /
  `is_set_to`

## See Also

- [`Series::is_same_size`](is_same_size.md): shape only.
- [`Series::equal`](equal.md): shape and exact values.
- [`Series::clone`](clone.md): the operation that guarantees `is_set_to()`
  becomes `false`.
- [`Series::contiguous`](contiguous.md): returns its input (still `set_to`)
  when already contiguous, a fresh buffer otherwise.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_same_size.html">
    <span class="label">Previous</span>
    <span class="title">is_same_size</span>
  </a>
  <a class="next-link" href="is_signed.html">
    <span class="label">Next</span>
    <span class="title">is_signed</span>
  </a>
</div>
