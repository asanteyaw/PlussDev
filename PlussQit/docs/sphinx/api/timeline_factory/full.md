<!-- pluss-hand-authored -->
# pluss::timeline::full

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signature

```cpp
Timeline pluss::timeline::full(integer_range sizes,
                               const Timeline& fill_value)
```

## Allocate A Shape From One Exact Instant

`full()` allocates a new owning `Timeline` with shape `sizes` and repeats one
scalar `fill_value` into every lane. It is the direct shape-first constructor
when the output geometry is known independently of its timestamp.

The fill tick is copied as `int64` epoch nanoseconds. No timestamp text is
reparsed and no floating-point conversion can remove low-order nanoseconds.
The output also inherits the fill value's timezone because that metadata
determines how every repeated instant is displayed and projected.

The fill must be rank zero. A one-element vector is not treated as a scalar:
rank is part of the contract, and accepting it would hide accidental geometry.
An absent scalar is valid and intentionally creates absent lanes.

The result is contiguous and independent of the fill value. Construction uses
storage proportional to the requested element count. For geometry already
carried by another `Timeline`, prefer `full_like()`.

## Example 1: An Empty Shape List Produces A Scalar

```cpp
const auto instant = pluss::timeline::from_epoch(
    1'709'208'000'123'456'789LL, "America/Edmonton");
const auto scalar = pluss::timeline::full({}, instant);
std::cout << pluss::note::print(
    pluss::timeline::format(scalar, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-02-29T05:00:00.123456789-07:00"
```

## Example 2: Fill A Matrix And Retain Timezone Metadata

```cpp
const auto instant = pluss::timeline::from_epoch(
    1'709'208'000'123'456'789LL, "America/Edmonton");
const auto matrix = pluss::timeline::full({2, 2}, instant);
std::cout << pluss::note::print(
    pluss::timeline::format(matrix, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-02-29T05:00:00.123456789-07:00", "2024-02-29T05:00:00.123456789-07:00"], 
  ["2024-02-29T05:00:00.123456789-07:00", "2024-02-29T05:00:00.123456789-07:00"]
]
```

## Example 3: Verify Exact Nanosecond Repetition

```cpp
const auto instant = pluss::timeline::from_epoch(
    1'709'208'000'123'456'789LL, "America/Edmonton");
const auto repeated = pluss::timeline::full({3}, instant);
std::cout << pluss::series::print(repeated.epoch_nanos()) << '\n';
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1709208000123456789, 1709208000123456789, 1709208000123456789]
```

## Example 4: Construct Intentional Absence

```cpp
const auto absent = pluss::timeline::full(
    {2}, pluss::timeline::from_epoch(pluss::Timeline::kNaT));
std::cout << pluss::note::print(
    pluss::timeline::format(absent, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["NaT", "NaT"]
```

This differs from `epoch()`, whose zero ticks are defined instants.

## Example 5: Zero-Length Axes Retain Metadata

```cpp
const auto instant = pluss::timeline::from_epoch(
    1'709'208'000'123'456'789LL, "America/Edmonton");
const auto empty = pluss::timeline::full({2, 0}, instant);
std::cout << pluss::timeline::print(empty) << '\n';
```

### Output

```text
Timeline(shape=[2, 0], tz=America/Edmonton)[  ]
[  ]
```

Even with no lanes, the result retains both shape and the fill timezone.

## Validation And Failure

The fill value must be scalar. Extents must be nonnegative, rank cannot exceed
three, and the element-count product must fit signed 64-bit shape arithmetic.
Shape validation occurs before allocation.

```cpp
try {
  (void)pluss::timeline::full({2}, pluss::timeline::epoch({1}));
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`epoch`](epoch.md): fill with the valid instant at epoch tick zero.
- [`full_like`](full_like.md): copy geometry from another `Timeline`.
- [`from_epoch`](from_epoch.md): supply every lane's exact tick separately.
