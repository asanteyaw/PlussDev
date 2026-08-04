<!-- pluss-hand-authored -->

# Series::take

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::take(const Series& index) const -> Series
```

## What It Does

Reads values by **flattened linear position** and reshapes the result to exactly
match `index`.

That is the entire contract:

- `self` is treated as one flat logical sequence
- `index` contains absolute positions into that sequence
- the output shape is `index.sizes()`

`take()` does not mean "take along this dimension." It means "read these flat
positions." If you need dimension-aware behavior, use [`gather()`](gather.md),
[`index_select()`](index_select.md), or [`index()`](index.md) instead.

## Parameters

**index** (`const Series&`)

The linear positions to read.

Important rules:

- `index` must be `Int64` on CPU.
- negative indices are allowed and count from the end
- repeated positions are allowed
- the index tensor must be on the same platform as the source
- the output shape is exactly `index.sizes()`

One subtle but important detail is what "flattened" means for non-contiguous
inputs. `take()` first resolves the source into its logical element order, then
reads flat positions from that order. It is not exposing arbitrary raw-stride
storage bytes.

## Returns

`Series`
    A new series with the same shape as `index`.

## Gradient

`take()` is differentiable in `self`.

Backward is a flat-position scatter-add:

- a source position never read gets gradient `0`
- a source position read once gets one contribution
- a source position read multiple times gets the sum of every contribution

That repeated-position accumulation is the most important write-side symmetry:
`take()` is the read counterpart of additive flat scattering.

## Examples

Example 1. `take()` reads from the flattened logical order of the source, then
arranges the answer into `index`'s shape.

```cpp
auto take_src = pluss::make_series(
    std::vector<double>{10, 11, 12, 20, 21, 22},
    {2, 3});
auto take_idx = pluss::make_series(std::vector<std::int64_t>{5, 0, 3, 1}, {2, 2});

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

Example 2. Negative indices count from the end of that same flattened logical order.

```cpp
auto negative_take_idx = pluss::make_series(std::vector<std::int64_t>{-1, -3, 0}, {3});
std::cout << take_src.take(negative_take_idx);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[22, 20, 10]
```

Example 3. When the source is non-contiguous, `take()` still works from logical
element order, not raw storage order.

```cpp
auto transposed = take_src.transpose(0, 1);
auto transposed_idx = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 3, 4, 5}, {2, 3});

std::cout << transposed.take(transposed_idx);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[10, 20, 11],
  [21, 12, 22]]
```

The transposed logical order is `[10, 20, 11, 21, 12, 22]`, which is why the
result differs from simply flattening the original `[2, 3]` source.

Example 4. Repeated positions accumulate gradient on the backward pass.

```cpp
auto tracked = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4});
tracked.tracks_slope_(true);

auto repeat_idx = pluss::make_series(std::vector<std::int64_t>{0, 0, 3}, {3});
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

Position `0` was read twice, so its gradient is `2`. Position `3` was read once,
so its gradient is `1`. Positions `1` and `2` were never read, so they remain `0`.

Example 5. On CPU, the public method requires an `Int64` index tensor.

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

## Notes

- `take()` is flat-addressed; it is not an alias for row/column selection.
- Repeated indices are legal and accumulate gradient.
- The result shape comes from `index`, not from the source.

## See Also

- [`Series::put_`](put_.md): in-place flat write counterpart.
- [`Series::gather`](gather.md): dimension-aware value collection.
- [`Series::index_select`](index_select.md): one shared index list along a named axis.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index_put_.html">
    <span class="label">Previous</span>
    <span class="title">index_put_</span>
  </a>
  <a class="next-link" href="put_.html">
    <span class="label">Next</span>
    <span class="title">put_</span>
  </a>
</div>
