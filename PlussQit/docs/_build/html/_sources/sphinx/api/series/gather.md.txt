<!-- pluss-hand-authored -->

# Series::gather

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::gather(std::int64_t dim, const Series& index) const -> Series
```

## What It Does

Collects one element per position in `index`, choosing which source element along
`dim` each output position pulls from. Every dimension except `dim` is walked
positionally in lockstep between `self` and `index`; only along `dim` does `index`'s
value pick which source row/column/... to read.

For a 2-D `self` and `dim = 1`, this is:

```text
out[i][j] = self[i][index[i][j]]
```

and symmetrically for `dim = 0`:

```text
out[i][j] = self[index[i][j]][j]
```

[`index_select`](index_select.md) is the special case of `gather` where the same
index list applies to *every* row (or column) uniformly — `gather` is strictly more
general, since `index` can vary per row.

## Parameters

**dim** (`std::int64_t`)

Which axis `index`'s values are looked up along. Every other axis is matched
positionally between `self` and `index` (`index`'s shape must match `self`'s shape
except possibly along `dim`).

**index** (`const Series&`)

Integer indices into `self` along `dim`. `index`'s values must be in
`[0, self.sizes()[dim])`. Indices may repeat — see the Gradient section for what
that means for the backward pass.

## Returns

`Series`
    A new `Series` with `index`'s shape, where `out[..., j, ...] = self[..., index[..., j, ...], ...]`
    (the substitution happening only along `dim`).

## Gradient

`gather` is differentiable in `self`. The backward pass is a `scatter_add`: each
output position's incoming gradient is added back onto the exact source position
`gather` read it from.

```{warning} Repeated indices sum their gradient, they don't overwrite it
If `index` reads the same source position more than once, that position's gradient
is the **sum** of every output position that read it — not just the last one, and
not split between them. See Example 2.
```

## Examples

Example 1. `dim = 1` on a 2×3 input: each output row picks two columns from the
same-numbered input row, independently per row.

```cpp
auto input = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {2, 3});
auto index = pluss::make_series(std::vector<std::int64_t>{0, 2, 1, 0}, {2, 2});
std::cout << input.gather(1, index);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 3],
  [5, 4]]
```

Row 0 of `index` is `[0, 2]`, so row 0 of the output is `input[0][0], input[0][2]`
= `1, 3`. Row 1 of `index` is `[1, 0]`, so row 1 of the output is
`input[1][1], input[1][0]` = `5, 4` — each row's index list is independent, which
is exactly what distinguishes `gather` from `index_select`.

Example 2. Gradient with a repeated index: position `0` is read twice, position `1`
never.

```cpp
auto input = pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}, {3});
input.tracks_slope_(true);

auto index = pluss::make_series(std::vector<std::int64_t>{0, 0, 2}, {3});
auto out = input.gather(0, index);
out.sum().reverse();
std::cout << "out=" << out << "\n";
std::cout << "input.sensitivity()=" << input.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 10, 30]
input.sensitivity()=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 0, 1]
```

`index = [0, 0, 2]` reads position `0` twice (into output positions 0 and 1) and
position `2` once; position `1` is never read. `out.sum()` gives every output
position gradient `1`, so position `0`'s two reads each contribute `1`, summing to
`2` — not `1`, and not `0.5` each. Position `1`, never read, correctly gets `0`.

## Notes

- `gather`'s `index` must match `self`'s shape on every axis except `dim` — it does
  not broadcast the way [`maximum`](maximum.md)/[`minimum`](minimum.md)'s
  `Series`-argument overloads do.
- Repeated indices are not an error; they just mean that source position's
  gradient accumulates from every output position that read it (Example 2).

## See Also

- [`Series::index_select`](index_select.md): the simpler "same index list for every
  row" special case of `gather`.
- [`Series::copy_`](copy_.md): the typical way a `gather`-consuming parameter gets
  its initial values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="copy_.html">
    <span class="label">Previous</span>
    <span class="title">copy_</span>
  </a>
  <a class="next-link" href="index_select.html">
    <span class="label">Next</span>
    <span class="title">index_select</span>
  </a>
</div>
