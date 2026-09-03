<!-- pluss-hand-authored -->

# Series::scatter

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::scatter(std::int64_t dim, const Series& index, const Series& src) const -> Series
```

## What It Does

`scatter()` writes values from `src` into a copy of `self`, using integer
positions from `index` along one chosen axis.

The easiest mental model is "axis-aware overwrite." Everywhere except the
chosen axis, `index` and `src` line up positionally with `self`. Along the
chosen axis, the integer stored in `index` says which destination slot should
receive the corresponding `src` value.

That makes `scatter()` the write-side sibling of [`gather()`](gather.md):

- `gather()` reads from chosen positions
- `scatter()` writes to chosen positions

It also explains why [`scatter_add()`](scatter_add.md) exists separately:

- `scatter()` overwrites
- `scatter_add()` accumulates

If repeated destinations should sum, use `scatter_add()`. If repeated
destinations should behave like successive assignments, use `scatter()`.

## Parameters

**dim** (`std::int64_t`)

The axis along which destination positions are chosen.

For a 2-D `Series`:

- `dim = 1` means "choose destination columns inside each row"
- `dim = 0` means "choose destination rows inside each column lane"

Negative dims normalize from the back in the usual `Series` style.

Changing `dim` changes the whole interpretation of the same `index` values, so
this parameter is never cosmetic.

**index** (`const Series&`)

The integer destination positions along `dim`.

Load-bearing rules:

- `index` must be integral
- `index` must already have the same rank as `self`
- `index` and `src` must match shape exactly
- on axes other than `dim`, `index` must already line up with `self`
- repeated positions are allowed

The exact-shape rule is the main difference from `scatter_add()`: this method
does not broadcast `src` to the `index` shape.

**src** (`const Series&`)

The source values to write.

Each source position pairs with the corresponding `index` position. The source
shape therefore describes the write pattern itself, not just the values.

## Returns

`Series`
    A new `Series` containing the scattered result. `self` is not mutated.

## Gradient

`scatter()` is differentiable in both `self` and `src`.

For unique destination positions:

- untouched lanes in `self` keep the incoming sensitivity
- overwritten lanes do not receive sensitivity through the original `self`
  value path
- each written `src` lane receives the sensitivity of the destination lane it
  filled

This is the mathematically natural rule for overwrite semantics: once a source
lane is replaced, the old destination value no longer influences the result at
that position.

If repeated destinations are conceptually supposed to accumulate contributions,
`scatter_add()` is the more faithful method. `scatter()` still permits repeated
targets, but the forward meaning is successive overwrite rather than reduction.

## Examples

Example 1. Along `dim = 1`, each row chooses which destination columns to
overwrite.

```cpp
auto base = pluss::make_series({
    {0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0},
});
auto index = pluss::make_series({
    {2LL, 0LL, 1LL},
    {1LL, 2LL, 0LL},
});
auto src = pluss::make_series({
    {10.0, 20.0, 30.0},
    {40.0, 50.0, 60.0},
});

std::cout << base.scatter(1, index, src);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[20, 30, 10],
  [60, 40, 50]]
```

Row `0` writes `10` into column `2`, `20` into column `0`, and `30` into
column `1`. Row `1` follows its own destination pattern.

Example 2. Switching to `dim = 0` changes the same idea into row selection.

```cpp
auto row_index = pluss::make_series({
    {0LL, 1LL, 0LL},
    {1LL, 0LL, 1LL},
});
auto row_src = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

std::cout << base.scatter(0, row_index, row_src);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 5, 3],
  [4, 2, 6]]
```

Now each column lane decides which row to write into. The method name is the
same, but the axis choice changes the whole write geometry.

Example 3. Negative dims are just alternate names for the same axis.

```cpp
std::cout << base.scatter(-1, index, src);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[20, 30, 10],
  [60, 40, 50]]
```

For a matrix, `-1` means the last axis, so this is the same write pattern as
`scatter(1, index, src)`.

Example 4. Repeated destinations are legal, but `scatter()` keeps overwrite
semantics rather than addition.

```cpp
auto duplicate_base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto duplicate_index = pluss::make_series({1LL, 1LL});
auto duplicate_src = pluss::make_series({5.0, 6.0});

pluss::series::print(
    std::cout,
    duplicate_base.scatter(0, duplicate_index, duplicate_src)) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10.0000, 6.0000, 30.0000, 40.0000]
```

Both writes target position `1`, so the later source value `6` is the final
stored value there.

Example 5. Reverse-mode on unique targets sends sensitivity through untouched
base lanes and through the written source lanes.

```cpp
auto grad_base = pluss::make_series({1.0, 2.0, 3.0, 4.0});
auto grad_src = pluss::make_series({5.0, 6.0});
auto grad_index = pluss::make_series({1LL, 3LL});

grad_base.tracks_slope_(true);
grad_src.tracks_slope_(true);

auto out = grad_base.scatter(0, grad_index, grad_src);
out.sum().reverse();

std::cout << "out=" << out << "\n";
std::cout << "dbase=" << grad_base.sensitivity() << "\n";
std::cout << "dsrc=" << grad_src.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 5, 3, 6]
dbase=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 0, 1, 0]
dsrc=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 1]
```

The scalar objective is

```text
L = 1 + 5 + 3 + 6
```

Positions `1` and `3` no longer depend on the original base values, so their
base sensitivity is `0`. Each source lane wrote one destination slot, so each
source sensitivity is `1`.

Example 6. `scatter()` requires `src` to match `index` shape exactly.

```cpp
auto broadcast_base = pluss::make_series({
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0},
});
auto broadcast_index = pluss::make_series({
    {0LL, 1LL, 2LL, 3LL},
    {0LL, 1LL, 2LL, 3LL},
});
auto row_src = pluss::make_series({{1.0, 2.0, 3.0, 4.0}});

try {
  std::cout << pluss::series::print(
      broadcast_base.scatter(1, broadcast_index, row_src));
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
scatter: index/src shape mismatch
```

This is one of the most important differences from `scatter_add()`, whose
source side can broadcast to the index shape.

## Gotchas And Failure Modes

- `scatter()` overwrites. If duplicate targets should sum, use
  [`scatter_add()`](scatter_add.md).
- `src` does not broadcast to `index` shape here.
- `index` must be integral and in range along the chosen axis.
- Changing `dim` changes destination meaning, not just syntax.
- If you need mutation instead of a copied result, use [`scatter_()`](scatter_.md).

## Summary

Read `scatter()` as "write `src` into a copy of `self` at indexed positions
along one axis."

- `dim` chooses the destination axis
- `index` names the destination positions
- `src` supplies one value per indexed write
- repeated targets overwrite rather than add

## See Also

- [`Series::scatter_`](scatter_.md): the in-place sibling.
- [`Series::scatter_add`](scatter_add.md): additive rather than overwrite
  semantics.
- [`Series::gather`](gather.md): the corresponding indexed-read primitive.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rsqrt.html">
    <span class="label">Previous</span>
    <span class="title">rsqrt</span>
  </a>
  <a class="next-link" href="scatter_.html">
    <span class="label">Next</span>
    <span class="title">scatter_</span>
  </a>
</div>
