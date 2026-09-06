<!-- pluss-hand-authored -->

# Series::movedim

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::movedim(std::int64_t source, std::int64_t destination) const -> Series
```

## What It Does

`movedim()` is an exact alias of [`moveaxis()`](moveaxis.md). It moves the axis
at `source` to `destination`, sliding the other axes along to fill the gap and
keeping their relative order. See [`moveaxis()`](moveaxis.md) for the full
description, the swap-versus-move distinction, examples, and gradient behavior.

Both names exist so code can use whichever term ("dim" or "axis") the
surrounding style prefers; they compile to the same call.

## Parameters

**source** (`std::int64_t`)

Current position of the axis to move. Negative counts from the end.

**destination** (`std::int64_t`)

Target position in the result. Negative counts from the end.

## Returns

`Series`
    A view with the axis relocated. Identical to `moveaxis(source, destination)`.

## Gradient

Differentiable, identical to [`moveaxis()`](moveaxis.md): the backward moves the
gradient's axis from `destination` back to `source`.

## Examples

Example 1. `movedim` and `moveaxis` produce the same result.

```cpp
std::vector<double> d(24);
for (int i = 0; i < 24; ++i) d[i] = i;
auto t3 = pluss::make_series(d, {2, 3, 4});

auto a = t3.movedim(0, 2);
auto b = t3.moveaxis(0, 2);
std::cout << "movedim=[";
for (auto x : a.sizes()) std::cout << x << ",";
std::cout << "]  moveaxis=[";
for (auto x : b.sizes()) std::cout << x << ",";
std::cout << "]";
```

### Output

```text
movedim=[3,4,2,]  moveaxis=[3,4,2,]
```

## Summary

`movedim` is a spelling of [`moveaxis()`](moveaxis.md) — same semantics, same
view result, same gradient.

## See Also

- [`Series::moveaxis`](moveaxis.md): the primary page for this operation.
- [`Series::swapdims`](swapdims.md) / [`Series::swapaxes`](swapaxes.md): the
  two-axis swap.
- [`Series::permute`](permute.md): reorder all axes at once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="moveaxis.html">
    <span class="label">Previous</span>
    <span class="title">moveaxis</span>
  </a>
  <a class="next-link" href="mul.html">
    <span class="label">Next</span>
    <span class="title">mul</span>
  </a>
</div>
