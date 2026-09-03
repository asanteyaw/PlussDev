<!-- pluss-hand-authored -->

# Series::diagonal_scatter

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::diagonal_scatter(const Series& src) const -> Series
Series::diagonal_scatter(const Series& src, std::int64_t diagonal) const -> Series
```

## What It Does

`diagonal_scatter()` returns a patched copy of a 2D `Series` after writing
`src` along one diagonal.

This is not an in-place update. The receiver stays unchanged and the method
returns a new result with the diagonal write applied.

The current verified root behavior is slightly more permissive than many users
first expect:

- `src` is flattened first
- `src` is converted to the receiver dtype if needed
- if `src` is shorter than the chosen diagonal, only the leading positions are
  overwritten
- if `src` is longer than the chosen diagonal, the extra values are silently
  ignored

That last rule is especially important. This method truncates rather than
throwing when the source is too long.

## Overload Map

**`diagonal_scatter(const Series& src)`**

Writes onto the main diagonal.

**`diagonal_scatter(const Series& src, std::int64_t diagonal)`**

Writes onto an offset diagonal.

## Parameters

**src** (`const Series&`)

The source values to write.

On the current root surface, `src` is flattened before the write. So the method
consumes `src` as a value stream rather than preserving its original rank.

**diagonal** (`std::int64_t`)

The target diagonal offset.

- `0` means the main diagonal
- positive values write above the main diagonal
- negative values write below the main diagonal

## Returns

`Series`
    A new 2D `Series` containing the diagonal write result.

## Examples

Example 1. The zero-argument overload patches the main diagonal by copy.

```cpp
auto square = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9},
}, pluss::ScalarType::Int64);

std::cout << square.diagonal_scatter(
    pluss::make_series({90, 80, 70}, pluss::ScalarType::Int64));
```

### Output

```text
pluss::Series(shape=[3,3], strides=[3,1], dtype=int64, platform=cpu, numel=9, contiguous=true)
data=[[90, 2, 3],
  [4, 80, 6],
  [7, 8, 70]]
```

Example 2. With an offset diagonal, the write uses only the positions that
actually exist. Extra `src` values are ignored.

```cpp
auto rectangular = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
}, pluss::ScalarType::Int64);

std::cout << rectangular.diagonal_scatter(
    pluss::make_series({50, 60, 70, 80}, pluss::ScalarType::Int64), 1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=[[1, 50, 3],
  [4, 5, 60]]
```

Only two positions exist on that `+1` diagonal, so only `50` and `60` are used.

Example 3. The receiver must be 2D.

```cpp
try {
  std::cout << pluss::make_series({1, 2, 3}, pluss::ScalarType::Int64).diagonal_scatter(
      pluss::make_series({9}, pluss::ScalarType::Int64)) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
diagonal_scatter: expected 2D input
```

## Gotchas And Failure Modes

- `diagonal_scatter()` is copy-returning. It does not mutate the receiver.
- The receiver must be 2D.
- `src` is flattened first on the current root surface.
- Overlong `src` does not throw. It truncates to the available diagonal length.
- Short `src` performs a partial diagonal overwrite and leaves the remaining
  diagonal positions unchanged.

## Summary

Use `diagonal_scatter()` when you want a copy-returning diagonal write. It is a
good fit for expression-style code, but remember the two big current rules:
2D receiver only, and flattened `src` with truncation instead of strict length
checking.

## See Also

- [`Series::diagonal`](diagonal.md): read a diagonal instead of writing one.
- [`Series::diag`](diag.md): dual-mode diagonal extract-or-embed surface.
- [`Series::copy_`](copy_.md): broad in-place overwrite surface instead of
  diagonal-only copy-returning write.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diagonal.html">
    <span class="label">Previous</span>
    <span class="title">diagonal</span>
  </a>
  <a class="next-link" href="diff.html">
    <span class="label">Next</span>
    <span class="title">diff</span>
  </a>
</div>
