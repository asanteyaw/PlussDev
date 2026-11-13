<!-- pluss-hand-authored -->

# Series::masked_select

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::masked_select(const Series& mask) const
```

## What It Does

`masked_select()` filters values out of `self` with a boolean mask and returns
the selected values as a flat 1-D `Series`.

That last part is the key mental model:

- the mask chooses which positions survive
- the result is packed densely in logical traversal order
- the original multi-dimensional layout is not preserved in the output

So `masked_select()` is not a masked copy of the original shape. It is a
compaction step.

## Parameters

**mask** (`const Series&`)

The boolean selector.

Public behavior that matters:

- `mask` must have boolean dtype
- `mask` may broadcast to the shape of `self`
- each `true` lane keeps the aligned value from `self`
- each `false` lane drops it entirely from the packed output

## Returns

`Series`
    A flat packed `Series` containing only the selected values.

If no positions match, the result is an empty 1-D `Series`.

## Examples

Example 1. The basic form keeps only the `true` positions and packs them into a
1-D result.

```cpp
auto values = pluss::make_series({
    {1, 2, 3},
    {4, 5, 6},
}, pluss::ScalarType::Int32);
auto mask = pluss::make_series({
    {true, false, true},
    {false, false, true},
}, pluss::ScalarType::Bool);

std::cout << values.masked_select(mask);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=int32, platform=cpu, numel=3, contiguous=true)
data=[1, 3, 6]
```

The survivors appear in logical traversal order: `(0,0)`, `(0,2)`, then
`(1,2)`.

Example 2. The mask may broadcast before selection happens.

```cpp
auto col_mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);

pluss::series::print(std::cout, values.masked_select(col_mask));
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[1, 3, 4, 6]
```

Because the 1-D mask broadcasts across both rows, the kept positions are
`(0,0)`, `(0,2)`, `(1,0)`, and `(1,2)`.

Example 3. A non-boolean mask is rejected.

```cpp
try {
  auto bad = pluss::make_series({1, 0, 1}, pluss::ScalarType::Int32);
  std::cout << values.masked_select(bad);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
masked_select: mask must be bool
```

Example 4. Broadcast incompatibility fails before any selection occurs.

```cpp
try {
  auto wrong_shape = pluss::make_series({true, false}, pluss::ScalarType::Bool);
  std::cout << values.masked_select(wrong_shape);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
broadcast: incompatible shapes
```

## Gotchas And Failure Modes

- The result is always packed flat, not shape-preserving.
- The mask must be boolean on the current public surface.
- Broadcast rules apply before compaction.

## Summary

Use `masked_select()` when a boolean mask should filter values down to a dense
1-D result.

## See Also

- [`Series::masked_scatter`](masked_scatter.md): write by mask instead of read
  by mask.
- [`Series::where`](where.md): branch between two full payloads instead of
  compacting survivors.
- [`Series::nonzero`](nonzero.md): return the matching coordinates rather than
  the matching values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="masked_scatter_.html">
    <span class="label">Previous</span>
    <span class="title">masked_scatter_</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
