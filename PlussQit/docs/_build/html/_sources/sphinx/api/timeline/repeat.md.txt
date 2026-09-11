<!-- pluss-hand-authored -->

# Timeline::repeat

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::repeat(std::int64_t times) const
```

## Tile Complete Temporal Blocks

`repeat()` copies the complete receiver block `times` times along axis 0. It is
block tiling, not element-by-element duplication. For `[A, B]`, `repeat(2)` is
`[A, B, A, B]`, not `[A, A, B, B]`.

For a non-scalar shape `[n, d1, ..., dk]`, the result shape is
`[n * times, d1, ..., dk]`. Trailing dimensions identify one record's geometry
and remain unchanged. A scalar is promoted to shape `[1]` before repetition,
so even `scalar.repeat(1)` returns a one-element vector.

| `times` | Result |
| ---: | --- |
| `0` | Empty axis 0 with trailing dimensions preserved |
| `1` | One materialized block; scalar shape becomes `[1]` |
| Greater than `1` | Blocks appear consecutively in receiver order |
| Negative | Rejected |

Example 1. The vector example makes block order visible.

```cpp
const auto row = pluss::timeline::date({"2024-01-01", "2024-01-02"});
std::cout << pluss::timeline::print(row.repeat(2)) << '\n';
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-01-01 00:00:00, 2024-01-02 00:00:00, 2024-01-01 00:00:00, 2024-01-02 00:00:00 ]
```

## Higher-Rank Shape Preservation

For matrices and 3-D timelines, repetition copies axis-0 slices. It never
flattens the source and never repeats independently along a trailing axis.

Example 2. Repeating one matrix row twice produces two rows with the same two
columns.

```cpp
const auto matrix = pluss::timeline::date({{"2024-01-01", "2024-01-02"}});
std::cout << pluss::timeline::print(matrix.repeat(2)) << '\n';
```

### Output

```text
Timeline(shape=[2, 2], tz=UTC)
[ 2024-01-01 00:00:00 2024-01-02 00:00:00 ]
[ 2024-01-01 00:00:00 2024-01-02 00:00:00 ]
```

## Logical Order And Ownership

The receiver may be strided. `repeat()` first follows logical indices, so
padding or skipped allocation positions are never copied into the result. The
result is newly allocated contiguous storage, retains the receiver's timezone,
and preserves `NaT` at every repeated position.

Time and payload storage are proportional to `numel() * times`. This is a real
copy, not a broadcasted view; use it when independent materialized records are
required.

Example 3. A stride-two source still repeats dates 1 and 2 in logical order.

```cpp
std::array<std::int64_t, 4> backing{
    1'704'067'200'000'000'000LL, 11,
    1'704'153'600'000'000'000LL, 22};
const std::array<std::int64_t, 1> shape{2};
const std::array<std::int64_t, 1> strides{2};
const auto strided = pluss::Timeline::from_buffer(
    backing.data(), shape, strides, {}, "UTC");
std::cout << pluss::timeline::print(strided.repeat(2)) << '\n';
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-01-01 00:00:00, 2024-01-02 00:00:00, 2024-01-01 00:00:00, 2024-01-02 00:00:00 ]
```

## Scalar Promotion And Zero Repetitions

Scalar promotion gives record-oriented code one uniform result rank. Zero
repetitions uses the same geometry as [`empty_like()`](empty_like.md): axis 0
is zero and all trailing dimensions survive.

Example 4. A scalar repeated once becomes `[1]`; a `[1,2]` matrix repeated zero
times becomes `[0,2]`.

```cpp
std::cout << pluss::timeline::print(
    pluss::timeline::date("2024-01-01").repeat(1)) << '\n';
std::cout << pluss::timeline::print(matrix.repeat(0)) << '\n';
```

### Output

```text
Timeline(shape=[1], tz=UTC)
[ 2024-01-01 00:00:00 ]
Timeline(shape=[0, 2], tz=UTC)
```

## Validation And Range Safety

`times` must be non-negative. Before allocation, `repeat()` checks both the
axis-0 multiplication and the total element-count multiplication. A request
whose shape or storage size cannot be represented is rejected rather than
wrapping into a smaller allocation.

Example 5. Negative and overflowing counts fail before producing a result.

```cpp
try {
  (void)row.repeat(-1);
} catch (const pluss::Error&) { std::cout << "rejected\n"; }
try {
  (void)row.repeat(std::numeric_limits<std::int64_t>::max());
} catch (const pluss::Error&) { std::cout << "rejected\n"; }
```

### Output

```text
rejected
rejected
```

## See Also

- [`Timeline::concat`](concat.md): assemble distinct compatible blocks.
- [`Timeline::empty_like`](empty_like.md): produce the zero-repeat geometry directly.
- [`Timeline::fill_like`](fill_like.md): preserve shape but replace every value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter_start_scalar.html"><span class="label">Previous</span><span class="title">quarter_start_scalar</span></a>
  <a class="next-link" href="round.html"><span class="label">Next</span><span class="title">round</span></a>
</div>
