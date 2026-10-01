<!-- pluss-hand-authored -->

# Series::sum_to_size

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::sum_to_size(pluss::msharp::RangeView::Int size) const
```

## Reversing A Broadcast By Summation

`sum_to_size()` reduces a Series to a shape from which it could have been
broadcast to the receiver's shape. It exists primarily for shape-correct
reverse rules: if a smaller operand was repeated implicitly across an
operation, its sensitivity must sum the repeated contributions rather than
retain the expanded shape.

Dimensions are aligned from the right. A target dimension is valid when it is
equal to the corresponding receiver extent or is `1`; a leading receiver
dimension may also be removed. Every removed dimension and every dimension
contracted to one is summed. Unlike a general [`sum()`](sum.md), the retained
axes and their singleton extents are inferred from the target shape itself.

For receiver shape `[2,3,4]`:

- `{1,3,1}` sums axes zero and two and keeps both singleton axes;
- `{3,4}` sums only the leading axis;
- `{}` sums every lane to a rank-zero scalar;
- `{2,3,4}` requests no reduction.

`size` cannot have more dimensions than the receiver, and an aligned target
extent other than `1` must equal the receiver extent. Negative extents are not
valid shapes. The result retains the receiver's dtype and platform.

## Ownership And Reverse Mode

The operation is out of place. A genuine reduction allocates a result; the
equal-shape case returns an equivalent Series without changing the receiver.
In reverse mode, output sensitivity is broadcast back across every reduced
axis, exactly reversing the forward sum-to-shape relation.

## Examples

### 1. Contract broadcast axes while retaining rank

```cpp
const auto cube = pluss::series::arange(
    1.0, 25.0, 1.0, pluss::ScalarType::Float64).reshape({2, 3, 4});
pluss::series::print(std::cout, cube.sum_to_size({1, 3, 1}));
```

#### Output

```text
Series(shape=[1,3,1], strides=[3,1,1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data[0, :, :] =
⎡  68.0000 ⎤
⎢ 100.0000 ⎥
⎣ 132.0000 ⎦
```

For each middle-axis position, all two leading lanes and all four trailing
lanes contribute to one sum.

### 2. Reduce to a scalar shape

```cpp
pluss::series::print(std::cout, cube.sum_to_size({}));
```

#### Output

```text
Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=300.0000
```

An empty target shape means rank zero, not an empty Series.

### 3. Request the existing shape

```cpp
pluss::series::print(std::cout, cube.sum_to_size({2, 3, 4}));
```

#### Output

```text
Series(shape=[2,3,4], strides=[12,4,1], dtype=float64, platform=cpu, numel=24, contiguous=true)
data[0, :, :] =
⎡ 1.0000  2.0000  3.0000  4.0000 ⎤
⎢ 5.0000  6.0000  7.0000  8.0000 ⎥
⎣ 9.0000 10.0000 11.0000 12.0000 ⎦

data[1, :, :] =
⎡ 13.0000 14.0000 15.0000 16.0000 ⎤
⎢ 17.0000 18.0000 19.0000 20.0000 ⎥
⎣ 21.0000 22.0000 23.0000 24.0000 ⎦
```

No axis is summed when every extent already agrees.

### 4. Broadcast sensitivity back to the receiver

```cpp
auto tracked = cube.clone();
tracked.tracks_slope_(true);
tracked.sum_to_size({1, 3, 1}).sum().reverse();
pluss::series::print(std::cout, tracked.sensitivity());
```

#### Output

```text
Series(shape=[2,3,4], strides=[12,4,1], dtype=float64, platform=cpu, numel=24, contiguous=true)
data[0, :, :] =
⎡ 1.0000 1.0000 1.0000 1.0000 ⎤
⎢ 1.0000 1.0000 1.0000 1.0000 ⎥
⎣ 1.0000 1.0000 1.0000 1.0000 ⎦

data[1, :, :] =
⎡ 1.0000 1.0000 1.0000 1.0000 ⎤
⎢ 1.0000 1.0000 1.0000 1.0000 ⎥
⎣ 1.0000 1.0000 1.0000 1.0000 ⎦
```

### 5. Reject a shape that could not broadcast to the receiver

```cpp
try {
  (void)cube.sum_to_size({2, 2, 4});
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
sum_to_size: incompatible target dimension
```

The middle extent `2` is neither the receiver extent `3` nor singleton `1`.

## See Also

- [`sum()`](sum.md) performs caller-selected reductions.
- [`broadcast_to()`](broadcast_to.md) performs the opposite shape expansion.
- [`reshape()`](reshape.md) changes shape without reducing values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="swapaxes.html">
    <span class="label">Next</span>
    <span class="title">swapaxes</span>
  </a>
</div>
