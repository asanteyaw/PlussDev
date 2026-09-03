<!-- pluss-hand-authored -->

# Series::roll

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::roll(std::int64_t shift) const -> Series
Series::roll(std::int64_t shift, std::int64_t dim) const -> Series
Series::roll(pluss::RangeView<std::int64_t> shifts,
             pluss::RangeView<std::int64_t> dims) const -> Series
```

## What It Does

`roll()` is a circular shift. Values moved past one edge reappear at the other;
none are discarded and the shape never changes. Positive shifts move toward
larger indices, while negative shifts move toward smaller indices. A shift is
reduced modulo the length of the axis it operates on.

The one-argument form first views all values as one flattened sequence, rotates
that sequence, then restores the original shape. The two-argument form keeps
every lane separate and rotates only `dim`. The list overload pairs shift(s)
with dimension(s), allowing a compound coordinate shift in one call.

## Overload Map

| Call | Movement |
| --- | --- |
| `x.roll(shift)` | one flattened circular sequence |
| `x.roll(shift, dim)` | each lane of one axis |
| `x.roll({shifts...}, {dims...})` | paired axis-local shifts |

## Parameters

**shift** (`std::int64_t`) is the signed displacement. Its effective value is
computed modulo the selected length.

**dim** (`std::int64_t`) selects the axis for the scalar axis-local form.
Negative dimensions count from the end.

**shifts** and **dims** are braced lists for the multi-axis form. Supply one
shift for each dimension, or one shift to apply to all listed dimensions.
Without `dims`, flattened rolling accepts exactly one shift.

## Returns

A new Series with the same shape and dtype. The source is unchanged.

## Gradient

Roll is a reversible coordinate permutation. Reverse-mode sensitivity follows
the inverse circular movement along the same flattened sequence or axes.

## Examples

Example 1. A flattened roll crosses row boundaries before the shape is restored.

```cpp
auto table = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << table.roll(2);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[5, 6, 1],
  [2, 3, 4]]
```

The flattened sequence `[1, 2, 3, 4, 5, 6]` becomes `[5, 6, 1, 2, 3, 4]`.

Example 2. Adding `dim=1` keeps rows independent; `-1` is a left shift.

```cpp
pluss::series::print(std::cout, table.roll(-1, 1));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[2, 3, 1],
  [5, 6, 4]]
```

Example 3. Multiple axes can shift in different directions together.

```cpp
std::cout << table.roll({1, -1}, {0, 1});
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[5, 6, 4],
  [2, 3, 1]]
```

Example 4. Large shifts wrap by the length of the chosen axis.

```cpp
std::cout << table.roll(4, 1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 1, 2],
  [6, 4, 5]]
```

Rows have length three, so `4 mod 3 = 1`: this is one right shift.

Example 5. Several flattened shifts are ambiguous and explicitly rejected.

```cpp
try { std::cout << table.roll({1, 2}); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
roll: flattened roll accepts exactly one shift; provide dims for multiple shifts
```

Example 6. Axis validation stays strict in the paired overload.

```cpp
try { std::cout << table.roll({1}, {2}); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
roll: dim out of range
```

## Gotchas And Failure Modes

- `roll(shift)` is not a final-axis roll; it is explicitly flattened.
- A list of shifts without dimensions must contain one element.
- Axis-local calls preserve lanes; flattened calls can cross their boundaries.
- Invalid axes throw rather than being ignored.

## Summary

Choose `roll(shift)` for one circular stream, `roll(shift, dim)` for separate
axis lanes, and the paired list form for compound shifts. All variants preserve
shape and values, differing only in the coordinate system they rotate.

## See Also

- [`Series::flip`](flip.md)
- [`Series::rot90`](rot90.md)
