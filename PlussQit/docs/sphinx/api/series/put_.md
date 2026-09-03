<!-- pluss-hand-authored -->

# Series::put_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::put_(const Series& index, const Series& source, bool accumulate = false) const -> Series&
```

## What It Does

Writes into the current `Series` by flat logical position and returns the same
handle.

`put_()` does not think in rows, columns, or named axes. It uses the same
addressing model as [`take()`](take.md):

- the destination is treated as one logical flat sequence
- `index` names positions inside that logical sequence
- writes land at those logical positions even when the destination is not
  contiguous

That makes `put_()` the right tool when your mental model is "scatter into flat
positions." If your mental model is instead "replace these rows," "write this
column," or "update a masked region," move to [`index_put_()`](index_put_.md)
or [`extract_put_()`](extract_put_.md).

One API quirk is worth stating directly: this public method is declared `const`
on the handle even though it mutates the underlying storage. On this surface,
handle constness does not imply value immutability.

## Parameters

**index** (`const Series&`)

The flat destination positions.

Important rules:

- on this surface, the index tensor must be `int64`
- negative indices are allowed and count from the end
- repeated positions are allowed
- the shape of `index` controls how many writes happen, not the shape of the
  destination

If `index` contains `3` elements, then `put_()` performs `3` flat writes.

**source** (`const Series&`)

The values to write.

Two cardinality modes are supported:

- scalar source: one value is reused for every indexed position
- non-scalar source: `source.numel()` must exactly match `index.numel()`

This is a one-value-per-write contract. `put_()` does not preserve higher-rank
structure from `source`; it simply consumes source elements in flat logical
order.

**accumulate** (`bool`)

Controls whether a write replaces or adds:

- `false`: replacement mode
- `true`: accumulation mode

This toggle matters most when the same destination position appears more than
once:

- in replacement mode, later writes win
- in accumulation mode, contributions are summed into the destination

## Returns

`Series&`
    The same `Series`, after mutation.

## Examples

Example 1. A scalar source broadcasts across every indexed flat position.

```cpp
auto put_idx = pluss::make_series({0LL, 2LL, 5LL});
auto flat = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});

flat.put_(put_idx, pluss::make_series(-1.0), false);
std::cout << flat;
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[-1, 1, -1, 3, 4, -1]
```

Example 2. Negative indices count from the end of the same logical flat order.

```cpp
auto negative_idx = pluss::make_series({-1LL, -3LL});
auto negative_values = pluss::make_series({99.0, 77.0});

flat.put_(negative_idx, negative_values, false);
std::cout << flat;
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[-1, 1, -1, 77, 4, 99]
```

At that moment `flat` was already `[-1, 1, -1, 3, 4, -1]`, so `-1` targeted
the last position and `-3` targeted the third-from-last position.

Example 3. With `accumulate=false`, repeated positions behave like repeated
replacements, so the last write wins.

```cpp
auto replace_idx = pluss::make_series({1LL, 1LL, 4LL});
auto replace_base = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
auto replace_values = pluss::make_series({50.0, 60.0, 90.0});

replace_base.put_(replace_idx, replace_values, false);
std::cout << replace_base;
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0, 60, 2, 3, 90, 5]
```

Example 4. With `accumulate=true`, repeated positions add into the destination
instead of replacing it.

```cpp
auto accum_base = pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0});
auto accum_values = pluss::make_series({10.0, 20.0, 30.0});

accum_base.put_(replace_idx, accum_values, true);
std::cout << accum_base;
```

### Output

```text
pluss::Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[0, 31, 2, 3, 34, 5]
```

Starting from `[0, 1, 2, 3, 4, 5]`:

- position `1` receives `+10` and `+20`, so `1` becomes `31`
- position `4` receives `+30`, so `4` becomes `34`

Example 5. Flat logical order still applies when the destination is
non-contiguous.

```cpp
auto transposed_base = pluss::make_series({
    {10.0, 11.0, 12.0},
    {20.0, 21.0, 22.0},
}).transpose(0, 1);

transposed_base.put_(
    pluss::make_series({0LL, 3LL, 5LL}),
    pluss::make_series({100.0, 300.0, 500.0}),
    false);

std::cout << transposed_base;
```

### Output

```text
pluss::Series(shape=[3,2], strides=[1,3], dtype=float64, platform=cpu, numel=6, contiguous=false)
data=[[100, 20],
  [11, 300],
  [12, 500]]
```

This is an important point: `put_()` follows logical flat order, not
"walk the current strides by hand."

Example 6. Out-of-range flat positions fail immediately.

```cpp
try {
  auto bad_pos = pluss::make_series({6LL});
  flat.put_(bad_pos, pluss::make_series(0.0), false);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
put: index out of range
```

Example 7. On this flat write surface, the index tensor must be `int64`.

```cpp
try {
  auto bad_idx = pluss::make_series(std::vector<std::int32_t>{0, 1}, {2});
  flat.put_(bad_idx, pluss::make_series(0.0), false);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
put: index must be int64
```

Example 8. A non-scalar source must supply exactly one value per indexed
position.

```cpp
try {
  auto bad_values = pluss::make_series({1.0, 2.0});
  flat.put_(put_idx, bad_values, false);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
put: src must be scalar or same numel as index
```

## Gotchas And Failure Modes

- `put_()` is flat, not axis-aware. If you are thinking in rows or columns,
  you are probably on the wrong method.
- `accumulate=false` does not reject duplicate positions. It keeps last-write
  behavior.
- `accumulate=true` changes semantics, not just performance. Repeated
  positions sum instead of overwrite.
- Negative indices are allowed, but they still must land inside the flattened
  logical extent.
- The strict `int64` rule applies here even though some higher-level write
  surfaces accept broader integer dtypes and normalize them internally.

## Summary

Use `put_()` when you want the write-side twin of [`take()`](take.md): flat
positions in, flat writes out. Think in logical element numbers, not axes.
Choose `accumulate=false` for overwrite behavior and `accumulate=true` for
scatter-add behavior. The moment your write intent becomes axis-aware or
selector-aware, graduate to [`index_put_()`](index_put_.md) or
[`extract_put_()`](extract_put_.md).

## See Also

- [`Series::take`](take.md): flat logical reads.
- [`Series::index_put_`](index_put_.md): explicit advanced-index writes.
- [`Series::extract_put_`](extract_put_.md): full selector-language writes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prod.html">
    <span class="label">Previous</span>
    <span class="title">prod</span>
  </a>
  <a class="next-link" href="reciprocal.html">
    <span class="label">Next</span>
    <span class="title">reciprocal</span>
  </a>
</div>
