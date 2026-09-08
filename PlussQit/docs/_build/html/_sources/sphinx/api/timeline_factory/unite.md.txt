<!-- pluss-hand-authored -->
# pluss::timeline::unite

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signature

```cpp
Timeline pluss::timeline::unite(timeline_range values)
```

The input may be a contiguous range of Timelines or a braced list.

## Assemble One Sequence In Argument Order

`unite()` appends the logical values of two or more Timelines in operand order
and returns one rank-one Timeline. Each operand is flattened in row-major
logical order before the next operand begins. It is sequence assembly, not
stacking along a selected axis, and there is no dimension parameter.

This distinction is important for shaped input. Joining two `2 x 2` Timelines
does not produce `2 x 4`, `4 x 2`, or `2 x 2 x 2`; it produces a vector of eight
instants. If a higher-rank arrangement is required, assemble the epoch ticks in
that shape deliberately and construct with `from_epoch(flat, sizes)`.

With exactly one operand, `unite()` is an identity operation: it returns a cheap
Timeline handle to the same logical value and preserves the operand's rank and
shape. It does not flatten or allocate a redundant join buffer. The observable
flattening contract begins with two operands.

## Timezone, Absence, And Empty Inputs

Every operand must carry exactly the same timezone label. Matching epoch
nanoseconds are not enough: the result has one display zone, so silently choosing
the first operand's metadata would make output depend on argument order. Convert
zones explicitly before joining.

Absent instants are retained in position. Empty Timelines are valid operands.
When two or more empty operands are joined, the result is an empty rank-one
Timeline because multi-input assembly always flattens. An empty operand list is
not meaningful and is rejected.

For two or more operands, the output owns newly allocated contiguous storage;
changes to any external buffer from which an operand was constructed cannot
change the joined result. Logical iteration is respected, so strided operands
are copied in their visible order rather than read as if contiguous.

## Example 1: Append A Scalar To A Vector

```cpp
const auto result = pluss::timeline::unite({
    pluss::timeline::date({"2024-01-01", "2024-01-02"}),
    pluss::timeline::date("2024-01-03")});

std::cout << pluss::note::print(
    pluss::timeline::format(result, "%F")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["2024-01-01", "2024-01-02", "2024-01-03"]
```

## Example 2: Multiple Operands Flatten

```cpp
const auto matrix = pluss::timeline::date({
    {"2024-01-01", "2024-01-02"},
    {"2024-01-03", "2024-01-04"}});
const auto result = pluss::timeline::unite({matrix, matrix});

std::cout << pluss::note::print(
    pluss::timeline::format(result, "%F")) << '\n';
```

### Output

```text
Note(shape=[8], dim=1, numel=8, encoding=utf8)
["2024-01-01", "2024-01-02", "2024-01-03", "2024-01-04", "2024-01-01", "2024-01-02", "2024-01-03", "2024-01-04"]
```

## Example 3: One Operand Retains Shape

```cpp
const auto matrix = pluss::timeline::date({
    {"2024-01-01", "2024-01-02"},
    {"2024-01-03", "2024-01-04"}});
const auto identity = pluss::timeline::unite({matrix});

std::cout << (identity.sizes() == matrix.sizes()) << '\n';
std::cout << identity.epoch_nanos().equal(matrix.epoch_nanos()) << '\n';
```

### Output

```text
true
true
```

## Example 4: Empty Operands Produce An Empty Vector

```cpp
const auto empty = pluss::timeline::unite({
    pluss::timeline::epoch({0, 2}),
    pluss::timeline::epoch({3, 0})});
std::cout << pluss::timeline::print(empty) << '\n';
```

### Output

```text
Timeline(shape=[0], tz=UTC)[  ]
```

The two input shapes are different, but both contribute zero flattened values.

## Example 5: Absence Keeps Its Position

```cpp
const auto result = pluss::timeline::unite({
    pluss::timeline::make({"2024-01-01", ""}),
    pluss::timeline::date("2024-01-02")});

std::cout << pluss::note::print(
    pluss::timeline::format(result, "%F")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["2024-01-01", "NaT", "2024-01-02"]
```

## Failures And Cost

`unite()` throws for an empty operand range or any timezone-label mismatch.
For multiple operands it performs one logical read per input element and one
output write, so time and storage are linear in the total `numel()`. The
single-operand identity path is constant-time handle copying.

## See Also

- [`from_epoch`](from_epoch.md): construct an explicitly shaped result from assembled ticks.
- [`select`](select.md): choose values positionally instead of appending them.
