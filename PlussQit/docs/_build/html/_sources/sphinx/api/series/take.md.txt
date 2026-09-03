<!-- pluss-hand-authored -->

# Series::take

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::take(const Series& index) const -> Series
```

## What It Does

`take()` reads values by flattened linear position and reshapes the result to
match `index`.

That is the whole contract:

- `self` is treated as one flat logical sequence
- `index` contains linear positions into that sequence
- the result shape is exactly `index.sizes()`

This method does not mean "take along this dimension." It means "read these
flat positions." If you need axis-aware behavior, use [`gather()`](gather.md),
[`index_select()`](index_select.md), or [`index()`](index.md) instead.

## Parameters

**index** (`const Series&`)

The linear positions to read.

Important rules:

- `index` must be `int64`
- negative indices are allowed and count from the end
- repeated positions are allowed
- the index tensor must live on the same platform as the source
- the output shape is exactly `index.sizes()`

One subtle but important detail is what "flattened" means for non-contiguous
inputs. `take()` resolves the source in its logical element order first, then
reads flat positions from that order. It is not exposing arbitrary raw storage
layout.

## Returns

`Series`
    A new `Series` with the same shape as `index`.

## Gradient

`take()` is differentiable in `self`.

Backward is a flat-position scatter-add:

- a source position never read gets gradient `0`
- a source position read once gets one contribution
- a source position read multiple times gets the sum of every contribution

That repeated-position accumulation is the most important backward rule on the
page.

## Examples

The examples below build from basic flat reads to negative indices,
non-contiguous sources, output-shape behavior, gradient accumulation, and
failure cases.

Example 1. `take()` reads from flat logical order and reshapes the answer to
match `index`.

```cpp
auto take_src = pluss::make_series({
    {10.0, 11.0, 12.0},
    {20.0, 21.0, 22.0},
});
auto take_idx = pluss::make_series({
    {5LL, 0LL},
    {3LL, 1LL},
});

std::cout << take_src.take(take_idx);
```

### Output

```text
pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[22, 10],
  [20, 11]]
```

The flattened source is `[10, 11, 12, 20, 21, 22]`, so the reads are
`5 -> 22`, `0 -> 10`, `3 -> 20`, and `1 -> 11`.

Example 2. Negative indices count from the end of that same flat logical order.

```cpp
auto negative_take_idx = pluss::make_series({-1LL, -3LL, 0LL});

std::cout << take_src.take(negative_take_idx);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[22, 20, 10]
```

Example 3. The output shape always comes from `index`, not from the source.

```cpp
auto one_idx = pluss::make_series({4LL});

std::cout << take_src.take(one_idx);
```

### Output

```text
pluss::Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[21]
```

Even though only one position is read, the result shape is `[1]` because that
is the shape of the index `Series`.

Example 4. On a non-contiguous source, `take()` still uses logical element
order, not raw storage order.

```cpp
auto transposed = take_src.transpose(0, 1);
auto transposed_idx = pluss::make_series({
    {0LL, 1LL, 2LL},
    {3LL, 4LL, 5LL},
});

std::cout << transposed.take(transposed_idx);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[10, 20, 11],
  [21, 12, 22]]
```

The transposed logical order is `[10, 20, 11, 21, 12, 22]`, which is why this
result differs from flattening the original `[2,3]` source.

Example 5. Repeated positions accumulate gradient on the backward pass.

```cpp
auto tracked = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {4});
tracked.tracks_slope_(true);

auto repeat_idx = pluss::make_series({0LL, 0LL, 3LL});
auto gathered = tracked.take(repeat_idx);
gathered.sum().reverse();

std::cout << "out=" << gathered << "\n";
std::cout << "dx=" << tracked.sensitivity();
```

### Output

```text
out=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 1, 4]
dx=pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2, 0, 0, 1]
```

Position `0` was read twice, so its gradient is `2`. Position `3` was read
once, so its gradient is `1`.

Example 6. Out-of-range positions fail explicitly.

```cpp
try {
  auto bad_pos = pluss::make_series({6LL});
  std::cout << take_src.take(bad_pos) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
take: index out of range
```

Example 7. The public method requires an `int64` index tensor.

```cpp
try {
  auto bad_idx = pluss::make_series(std::vector<std::int32_t>{0, 1}, {2});
  std::cout << take_src.take(bad_idx) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
take: index must be int64
```

## Gotchas And Failure Modes

- `take()` is flat-addressed, not axis-aware.
- The result shape comes from `index`, not from the source.
- Negative indices are legal and count from the end.
- Non-contiguous sources are read in logical element order.
- Repeated positions accumulate gradient.
- Out-of-range and wrong-dtype indices fail explicitly.

## Summary

Use `take()` when your selector is best understood as a list of flat logical
positions.

- Use it for flat linear reads.
- Use negative indices when end-relative flat positions are clearer.
- Expect output shape to mirror the index `Series`.
- Use axis-aware methods instead when dimension meaning matters.

The key habit is to ask one question first: "am I selecting by flat position or
by axis meaning?" If the answer is flat position, `take()` is the right
surface.

## See Also

- [`Series::put_`](put_.md): in-place flat write counterpart.
- [`Series::gather`](gather.md): dimension-aware value collection.
- [`Series::index_select`](index_select.md): one shared index list along a
  named axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="take_along_dim.html">
    <span class="label">Next</span>
    <span class="title">take_along_dim</span>
  </a>
</div>
