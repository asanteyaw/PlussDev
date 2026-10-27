<!-- pluss-hand-authored -->
# pluss::timeline::from_epoch

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signatures

```cpp
Timeline pluss::timeline::from_epoch(std::int64_t value,
                                     std::string_view timezone = "UTC")
Timeline pluss::timeline::from_epoch(integer_data values,
                                     std::string_view timezone = "UTC")
Timeline pluss::timeline::from_epoch(integer_range flat, integer_range sizes,
                                     std::string_view timezone = "UTC")
Timeline pluss::timeline::from_epoch(const Series& values,
                                     std::string_view timezone = "UTC")
```

## Preserve Exact Epoch Nanoseconds

`from_epoch()` is the lossless integer entrance to `Timeline`. Every value is
interpreted as a signed count of nanoseconds from
`1970-01-01T00:00:00.000000000Z`. Positive values follow the epoch, negative
values precede it, and zero is the epoch itself.

Use this factory for database ticks, binary protocols, memory-mapped integer
columns, or a round trip through `epoch_nanos()`. It never passes values through
floating point, so adjacent nanoseconds remain distinct even beyond the exact
integer range of a binary floating representation.

`timezone` is presentation metadata. It changes which civil fields and offset
are displayed, but not the supplied ticks. The name is validated immediately.
The reserved value `Timeline::kNaT` deliberately creates an absent instant;
every other signed 64-bit tick is an instant.

The `Series` overload is the shape-preserving bridge from numeric work into
temporal work. Its dtype must already be `int64`; the factory rejects other
dtypes instead of silently truncating floating values or reinterpreting a
different integer width. Noncontiguous input is read in logical row-major
order, and the resulting Timeline owns its epoch data independently.

## Example 1: Exactness Beyond `2^53`

```cpp
constexpr std::int64_t tick = 9'007'199'254'740'993LL;
const auto t = pluss::timeline::from_epoch(tick);
pluss::series::print(std::cout, t.epoch_nanos());
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=9007199254740993
```

Scalar input creates a rank-zero timeline and returns the identical integer.

## Example 2: Before, At, And Absent From The Epoch

```cpp
const auto t = pluss::timeline::from_epoch(
    {-1LL, 0LL, pluss::Timeline::kNaT});
std::cout << pluss::note::print(
    pluss::timeline::format(t, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["1969-12-31T23:59:59.999999999+00:00", "1970-01-01T00:00:00.000000000+00:00", "NaT"]
```

Negative nanoseconds use ordinary floor-based temporal meaning: `-1` is the
last nanosecond before the epoch, not a truncated zero.

## Example 3: Nested Integer Geometry

```cpp
const auto matrix = pluss::timeline::from_epoch({{0LL, 1LL}, {2LL, 3LL}});
pluss::series::print(std::cout, matrix.epoch_nanos());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 0 1 ⎤
⎣ 2 3 ⎦
```

Nested literals must be rectangular and may express up to rank three.

## Example 4: Preserve A Series Shape

```cpp
const auto ticks = pluss::make_series(
    {{std::int64_t{0}, std::int64_t{1}},
     {std::int64_t{2}, std::int64_t{3}}});
const auto matrix = pluss::timeline::from_epoch(ticks);
pluss::series::print(std::cout, matrix.epoch_nanos());
```

### Output

```text
Series(shape=[2,2], strides=[2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=
⎡ 0 1 ⎤
⎣ 2 3 ⎦
```

This overload preserves rank and shape; it does not flatten a matrix into a
one-dimensional Timeline.

## Example 5: A Timezone Changes The View, Not The Tick

```cpp
const auto local =
    pluss::timeline::from_epoch(0LL, "America/Edmonton");
std::cout << pluss::note::print(
    pluss::timeline::format(local, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"1969-12-31T17:00:00.000000000-07:00"
```

The local calendar shows the preceding date, but `epoch_nanos()` remains zero.

## Example 6: Runtime Flat Data With Three-Dimensional Shape

```cpp
const std::vector<std::int64_t> flat{0, 1, 2, 3};
const auto cube = pluss::timeline::from_epoch(flat, {1, 2, 2});
pluss::series::print(std::cout, cube.epoch_nanos());
```

### Output

```text
Series(shape=[1,2,2], strides=[4,2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data[0, :, :] =
⎡ 0 1 ⎤
⎣ 2 3 ⎦
```

The product of `sizes` must equal `flat.size()`. The factory copies values into
contiguous owned storage; use the buffer bridge when existing strided storage
must be shared instead.

## Round Trips And Failures

`from_epoch(t.epoch_nanos().host<std::int64_t>(), t.sizes())` preserves every
tick and shape, but the new timezone defaults to UTC unless supplied. Unknown
timezone names, negative extents, rank above three, ragged nested input, and
shape/cardinality mismatch throw before a usable result is returned.

```cpp
try {
  (void)pluss::timeline::from_epoch(0LL, "Mars/Olympus_Mons");
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`make`](make.md): parse textual timestamps.
- [`epoch`](epoch.md): allocate a shape filled with tick zero.
- [`full`](full.md): allocate a shape filled with any scalar instant.
- [`Timeline::from_buffer`](../timeline/from_buffer.md): share external tick storage.
