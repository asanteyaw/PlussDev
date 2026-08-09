<!-- pluss-hand-authored -->

# Series::gather

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::gather(std::int64_t dim, const Series& index) const -> Series
```

## What It Does

Reads values from `self` by looking up positions along one chosen axis.

`gather()` is easiest to understand as "positional alignment everywhere except
one axis." Along every axis other than `dim`, `self` and `index` move in
lockstep. Along `dim`, the integer stored in `index` decides which source
position to read.

That gives `gather()` a different role from [`index_select`](index_select.md):

- `index_select()` reuses one index list everywhere
- `gather()` can vary the lookup pattern from row to row, column to column, or
  more generally from lane to lane

## Parameters

**dim** (`std::int64_t`)

The axis along which lookup happens.

If `dim == 1` for a 2-D series, then each output position chooses a column from
its own row. If `dim == 0`, each output position chooses a row for its own
column slot.

**index** (`const Series&`)

Integer positions to read from `self` along `dim`.

Important constraints:

- `index` must be integral
- `index` does not broadcast the way binary elementwise ops do
- every non-`dim` axis must already line up positionally with `self`
- repeated positions are allowed

## Returns

`Series`
    A new series with the same shape as `index`, filled from `self` according
    to the requested lookup positions.

## Gradient

`gather()` is differentiable in `self`.

Its backward is a `scatter_add`: every output position sends its incoming
gradient back to the exact source position that supplied its value.

That means repeated indices do not overwrite one another in the backward pass.
They accumulate.

## Examples

Example 1. `dim = 1` means "for each row, pick these columns."

```cpp
auto table = pluss::make_series({
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
});

auto gather_cols = pluss::make_series({
    {0, 2},
    {1, 0},
});

std::cout << table.gather(1, gather_cols);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 3],
  [5, 4]]
```

Row `0` asked for columns `0` and `2`. Row `1` asked for columns `1` and `0`.
That row-specific variability is exactly why `gather()` exists.

Example 2. `dim = 0` means "for each column slot, pick which row to read."

```cpp
auto gather_rows = pluss::make_series({
    {0, 1, 0},
    {1, 0, 1},
});

std::cout << table.gather(0, gather_rows);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[1, 5, 3],
  [4, 2, 6]]
```

The axis toggle changes the meaning completely even though the source table is
the same.

Example 3. Repeated indices accumulate gradient on the source position they
read from more than once.

```cpp
auto input = pluss::make_series({10.0, 20.0, 30.0}, {3});
input.tracks_slope_(true);

auto repeated = pluss::make_series({0, 0, 2}, {3});
auto gathered = input.gather(0, repeated);
gathered.sum().reverse();
std::cout << "out=" << gathered << "\n";
std::cout << "dx=" << input.sensitivity() << "\n";
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 10, 30]
dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2, 0, 1]
```

Position `0` was read twice, so it receives gradient `2`. Position `1` was
never read, so it receives `0`.

## Notes

- `gather()` is the more general indexed-read primitive.
- If the same index list should apply uniformly everywhere, use
  [`index_select`](index_select.md) instead.
- The output shape follows `index`, not `self`.

## See Also

- [`Series::index_select`](index_select.md): the uniform-index special case.
- [`Series::take`](take.md): flat linear-index reading rather than axis-based
  gathering.

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
