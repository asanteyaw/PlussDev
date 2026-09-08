<!-- pluss-hand-authored -->

# Timeline::concat

[Back to Timeline API Reference](../../timeline_api_reference)

## Signatures

```cpp
Timeline Timeline::concat(const Timeline& other) const
Timeline Timeline::concat(const std::vector<Timeline>& others) const
```

## Assemble Records Along Axis Zero

`concat()` constructs one `Timeline` by placing complete temporal blocks one
after another along axis 0. It is structural assembly: values are not sorted,
parsed again, compared, or made distinct. Every stored instant, including
`NaT`, keeps its position inside its source block.

Use the single-value overload when appending one block. Use the vector overload
when the complete sequence of blocks is already available; it validates the
whole sequence and allocates the destination once, avoiding a chain of
increasingly large intermediate timelines.

## Shape Contract

For non-scalar inputs, every operand must have the same rank and the same
dimensions after axis 0. If `A` has shape `[a, d1, ..., dn]` and `B` has shape
`[b, d1, ..., dn]`, their result has shape `[a + b, d1, ..., dn]`.

This makes each axis-0 slice a record whose trailing shape is fixed. A `[2, 3]`
block can join a `[5, 3]` block, but not a `[5, 2]` block and not a rank-one
`[3]` vector.

Scalars are the deliberate exception. Each scalar is promoted to a one-element
sequence, so scalar plus scalar produces shape `[2]`. Scalar promotion does not
make a scalar compatible with a matrix: after promotion their ranks still
differ.

| Receiver | Added block | Result |
| --- | --- | --- |
| scalar | scalar | `[2]` |
| `[a]` | `[b]` | `[a + b]` |
| `[a, d]` | `[b, d]` | `[a + b, d]` |
| `[a, d, e]` | `[b, d, e]` | `[a + b, d, e]` |

Example 1. Joining two one-row matrices retains the column axis; it does not
flatten the four instants into shape `[4]`.

```cpp
const auto jan12 = pluss::timeline::date({{"2024-01-01", "2024-01-02"}});
const auto jan34 = pluss::timeline::date({{"2024-01-03", "2024-01-04"}});
std::cout << pluss::timeline::print(jan12.concat(jan34)) << '\n';
```

### Output

```text
Timeline(shape=[2, 2], tz=UTC)
[ 2024-01-01 00:00:00 2024-01-02 00:00:00 ]
[ 2024-01-03 00:00:00 2024-01-04 00:00:00 ]
```

## Joining Several Blocks

The vector overload treats `others` in iteration order. The receiver is always
first, followed by `others[0]`, `others[1]`, and so on. Passing an empty vector
is valid and returns the receiver's values and shape in fresh contiguous
storage.

Example 2. Three one-row matrices become a three-row matrix in call order.

```cpp
const auto jan56 = pluss::timeline::date({{"2024-01-05", "2024-01-06"}});
std::cout << pluss::timeline::print(jan12.concat({jan34, jan56})) << '\n';
```

### Output

```text
Timeline(shape=[3, 2], tz=UTC)
[ 2024-01-01 00:00:00 2024-01-02 00:00:00 ]
[ 2024-01-03 00:00:00 2024-01-04 00:00:00 ]
[ 2024-01-05 00:00:00 2024-01-06 00:00:00 ]
```

Example 3. Scalar promotion and an explicitly empty block list are separate
call forms.

```cpp
const auto joined_scalars = pluss::timeline::date("2024-01-01")
                                .concat(pluss::timeline::date("2024-01-02"));
std::cout << pluss::timeline::print(joined_scalars) << '\n';
std::cout << pluss::timeline::print(
    jan12.concat(std::vector<pluss::Timeline>{})) << '\n';
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-01-01 00:00:00, 2024-01-02 00:00:00 ]
Timeline(shape=[1, 2], tz=UTC)
[ 2024-01-01 00:00:00 2024-01-02 00:00:00 ]
```

## Timezone And Storage Rules

All operands must carry the same timezone name. `concat()` does not silently
choose one display calendar, even when differently labelled timelines contain
the same UTC instants. Convert deliberately with `tz_convert()` before joining.
The result keeps the receiver's timezone.

Inputs may be contiguous or strided. Values are copied in logical index order,
not raw allocation order, and the result is newly allocated contiguous storage.
The operation is linear in the total number of copied instants and requires
storage for exactly that total.

Example 4. A stride-two view contributes its logical dates, not the padding
values between them.

```cpp
std::array<std::int64_t, 4> backing{
    1'704'067'200'000'000'000LL, 11,
    1'704'153'600'000'000'000LL, 22};
const std::array<std::int64_t, 1> shape{2};
const std::array<std::int64_t, 1> strides{2};
const auto strided = pluss::Timeline::from_buffer(
    backing.data(), shape, strides, {}, "UTC");
std::cout << pluss::timeline::print(
    strided.concat(pluss::timeline::date("2024-01-03"))) << '\n';
```

### Output

```text
Timeline(shape=[3], tz=UTC)
[ 2024-01-01 00:00:00, 2024-01-02 00:00:00, 2024-01-03 00:00:00 ]
```

## Absent Instants

`NaT` is ordinary payload for structural assembly. It is neither removed nor
expanded and does not prevent neighboring instants from being copied.

Example 5. The absent middle instant retains its exact position.

```cpp
const auto values = pluss::timeline::make({"2024-01-01", ""});
std::cout << pluss::timeline::print(
    values.concat(pluss::timeline::date("2024-01-03"))) << '\n';
```

### Output

```text
Timeline(shape=[3], tz=UTC)
[ 2024-01-01 00:00:00, NaT, 2024-01-03 00:00:00 ]
```

## Failure Modes

Validation rejects different ranks after scalar promotion, unequal trailing
dimensions, unequal timezone names, an overflowing axis-0 extent, or a total
element count that cannot be represented. The vector overload checks every
operand before allocating and copying the destination.

Example 6. Trailing-shape, timezone, and rank failures are distinct structural
errors.

```cpp
const std::vector<std::string> values{
    "2024-01-03", "2024-01-04", "2024-01-05"};
try {
  (void)jan12.concat(pluss::timeline::date(values, {3, 1}));
} catch (const pluss::Error&) { std::cout << "rejected\n"; }
try {
  (void)jan12.concat(jan34.tz_convert("America/Edmonton"));
} catch (const pluss::Error&) { std::cout << "rejected\n"; }
try {
  (void)jan12.concat(pluss::timeline::date("2024-01-03"));
} catch (const pluss::Error&) { std::cout << "rejected\n"; }
```

### Output

```text
rejected
rejected
rejected
```

## See Also

- [`Timeline::repeat`](repeat.md): tile one complete block several times.
- [`Timeline::empty_like`](empty_like.md): create a zero-record structural identity.
- [`pluss::timeline::unite`](../timeline_factory/unite.md): factory-oriented assembly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ceil.html"><span class="label">Previous</span><span class="title">ceil</span></a>
  <a class="next-link" href="copy_ticks.html"><span class="label">Next</span><span class="title">copy_ticks</span></a>
</div>
