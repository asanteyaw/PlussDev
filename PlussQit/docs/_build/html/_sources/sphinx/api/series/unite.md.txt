<!-- pluss-hand-authored -->

# Series::unite

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::unite(pluss::RangeView<Series> others,
              std::int64_t dim = 0,
              bool strict_dtype = true) const -> Series
```

## What It Does

Concatenates `self` with one or more peer series along an existing axis.

Conceptually, `unite()` is `cat`: it keeps rank unchanged, enlarges one
existing axis, and copies each input block into the output in order. `self`
always contributes the first block; the entries in `others` follow after it.

This is not a view op. The result is a newly materialized series with its own
contiguous storage. That difference matters if you are comparing it to
[`transpose()`](transpose.md), [`slice()`](slice.md), or the traversal family,
which often preserve aliasing instead of allocating.

## Parameters

**others** (`pluss::RangeView<Series>`)

The additional series to concatenate after `self`.

All inputs must:

- live on the same platform
- have the same rank
- have matching sizes on every axis except `dim`

Their size along `dim` is allowed to differ; that is the axis being extended.

**dim** (`std::int64_t`)

Which existing axis to concatenate along. Negative indexing is supported.

For a `2 x 2` series:

- `dim = 0` appends more rows
- `dim = 1` or `dim = -1` appends more columns

**strict_dtype** (`bool`)

Controls dtype compatibility.

- `true`: every input must already have exactly the same dtype
- `false`: inputs are promoted/coerced to a common dtype before concatenation

This toggle changes both ergonomics and output dtype. It does not change how
shapes are validated.

## Returns

`Series`
    A new series containing `self` followed by `others` along `dim`.

## Gradient

`unite()` is differentiable in every tracked input. Backward splits the
outgoing sensitivity into contiguous slices along `dim` and routes each slice
back to the input that produced it.

That rule is exact and structural:

- the gradient for `self` is the first slice
- the gradient for `others[0]` is the next slice
- and so on

No averaging or value-based routing is involved; the split is determined purely
by each input's width along the concatenated axis.

## Examples

Example 1. Concatenate along rows (`dim = 0`).

```cpp
auto left = pluss::make_series({1, 2, 3, 4}, {2, 2});
auto right = pluss::make_series({10, 20, 30, 40}, {2, 2});
std::cout << left.unite({right}, 0, true);
```

### Output

```text
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2],
  [3, 4],
  [10, 20],
  [30, 40]]
```

Rank stayed `2`, but axis `0` grew from length `2` to length `4`. `left`
occupies the first two rows, `right` the next two.

Example 2. Negative dimensions work the same way they do elsewhere on the
surface.

```cpp
std::cout << left.unite({right}, -1, true);
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2, 10, 20],
  [3, 4, 30, 40]]
```

`dim = -1` means "last axis", so this appends columns rather than rows.

Example 3. With `strict_dtype = true`, dtype mismatch is an error.

```cpp
try {
  auto ints = pluss::make_series({7, 8, 9, 10}, {2, 2});
  std::cout << left.unite({ints}, 0, true);
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
unite: dtype mismatch (set strict_dtype=false to enable promotion/coercion)
```

This overload is intentionally strict because it protects downstream code from
silent promotion when exact dtype parity matters.

Example 4. Setting `strict_dtype = false` enables promotion/coercion.

```cpp
auto ints = pluss::make_series({7, 8, 9, 10}, {2, 2});
std::cout << left.unite({ints}, 0, false);
```

### Output

```text
pluss::Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2],
  [3, 4],
  [7, 8],
  [9, 10]]
```

The result is `float64`, because that is the promoted common dtype between
`left` (`float64`) and `ints` (`int64`).

Example 5. Backward splits the output sensitivity into one slice per input.

```cpp
auto tracked_left = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2});
auto tracked_right = pluss::make_series({10.0, 20.0, 30.0, 40.0}, {2, 2});
tracked_left.tracks_slope_(true);
tracked_right.tracks_slope_(true);

auto united = tracked_left.unite({tracked_right}, 0, true);
united.sum().reverse();
std::cout << "dleft=" << tracked_left.sensitivity() << "\n";
std::cout << "dright=" << tracked_right.sensitivity() << "\n";
```

### Output

```text
dleft=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 1],
  [1, 1]]
dright=pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[1, 1],
  [1, 1]]
```

`united.sum()` gives every output position gradient `1`, so each input receives
exactly the slice it contributed. If the inputs had different widths along
`dim`, each would still receive the correctly sized slice.

## Notes

- `unite()` changes size along an existing axis; it does not create a new axis.
- `strict_dtype` is the decisive behavior toggle on this method.
- The output is materialized, not aliased.

## See Also

- [`Series::pile`](pile.md): the stack-style sibling that inserts a new axis.
- [`Series::flatten`](flatten.md): reshape-based axis collapsing rather than
  concatenation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unionize_along.html">
    <span class="label">Previous</span>
    <span class="title">unionize_along</span>
  </a>
  <a class="next-link" href="unsqueeze.html">
    <span class="label">Next</span>
    <span class="title">unsqueeze</span>
  </a>
</div>
