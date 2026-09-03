<!-- pluss-hand-authored -->

# Series::pile

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::pile(pluss::RangeView<Series> others,
             std::int64_t dim = 0) const -> Series
```

## What It Does

Stacks `self` with one or more peer series along a new axis.

Conceptually, `pile()` is `stack`: unlike [`unite()`](unite.md), it does not
extend an existing axis. Instead, it inserts a new axis of length equal to the
number of inputs (`1 + others.size()`) and places each input in one slice along
that axis.

`self` always occupies slice `0` on the new axis, then `others` fill slices
`1`, `2`, and so on.

## Parameters

**others** (`pluss::RangeView<Series>`)

The additional series to stack after `self`.

All inputs must:

- live on the same platform
- have exactly the same shape
- have exactly the same dtype

Unlike [`unite()`](unite.md), there is no dtype-promotion toggle here. Stacking
is strict.

**dim** (`std::int64_t`)

Where the new axis should be inserted. Negative indexing is supported against
the output rank (`self.dim() + 1`).

For a `2 x 2` input:

- `dim = 0` yields `[n, 2, 2]`
- `dim = 1` yields `[2, n, 2]`
- `dim = -1` yields `[2, 2, n]`

## Returns

`Series`
    A new series with one extra axis containing the stacked inputs.

## Gradient

`pile()` is differentiable in every tracked input. Backward selects the slice
of the outgoing sensitivity corresponding to each input's position on the new
axis and routes that slice back to the input.

This is the exact structural mirror of the forward op:

- input `0` receives `out_sens.select(dim, 0)`
- input `1` receives `out_sens.select(dim, 1)`
- and so on

## Examples

Example 1. Stack two `2 x 2` inputs along a new leading axis.

```cpp
auto left = pluss::make_series({1, 2, 3, 4}, {2, 2});
auto right = pluss::make_series({10, 20, 30, 40}, {2, 2});
std::cout << left.pile({right}, 0);
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[1, 2],
  [3, 4]],
  [[10, 20],
  [30, 40]]]
```

The new axis has length `2`: slice `0` is `left`, slice `1` is `right`.

Example 2. A trailing new axis gives a different layout story even though the
same two inputs participate.

```cpp
std::cout << left.pile({right}, -1);
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[1, 10],
  [2, 20]],
  [[3, 30],
  [4, 40]]]
```

Now the new axis is the last one, so each original scalar position has become
the pair `[left_value, right_value]`.

Example 3. Shape mismatch is rejected.

```cpp
try {
  auto bad_shape = pluss::make_series({5, 6, 7, 8, 9, 10}, {3, 2});
  std::cout << left.pile({bad_shape}, 0);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
stack: shape mismatch
```

This method is intentionally stricter than [`unite()`](unite.md): because it is
creating a new axis rather than extending one, every preexisting axis must
already agree exactly.

Example 4. Backward selects each input's slice along the new axis.

```cpp
auto tracked_left = pluss::make_series(
    {1.0, 2.0, 3.0, 4.0}, {2, 2});
auto tracked_right = pluss::make_series(
    {10.0, 20.0, 30.0, 40.0}, {2, 2});
tracked_left.tracks_slope_(true);
tracked_right.tracks_slope_(true);

auto stacked = tracked_left.pile({tracked_right}, 1);
stacked.sum().reverse();
std::cout << "dleft=" << tracked_left.sensitivity() << "\n";
std::cout << "dright=" << tracked_right.sensitivity() << "\n";
```

### Output

```text
dleft=pluss::Series(shape=[2,2], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=false)
data=[[1, 1],
  [1, 1]]
dright=pluss::Series(shape=[2,2], strides=[4,1], dtype=float64, platform=cpu, numel=4, contiguous=false)
data=[[1, 1],
  [1, 1]]
```

The sensitivities are all ones because `stacked.sum()` touched every output
position once. The key point is structural: each parent receives the slice of
the stacked output that it originally occupied.

## Notes

- `pile()` inserts a new axis; [`unite()`](unite.md) does not.
- All input shapes must match exactly.
- The result is materialized, not aliased.

## See Also

- [`Series::unite`](unite.md): concatenate along an existing axis instead.
- [`Series::unsqueeze`](unsqueeze.md): the conceptual one-input version of
  creating a new axis before alignment.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="permute.html">
    <span class="label">Previous</span>
    <span class="title">permute</span>
  </a>
  <a class="next-link" href="pos.html">
    <span class="label">Next</span>
    <span class="title">pos</span>
  </a>
</div>
