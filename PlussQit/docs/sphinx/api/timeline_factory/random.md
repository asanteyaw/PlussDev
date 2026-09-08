<!-- pluss-hand-authored -->
# pluss::timeline::random

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signature

```cpp
Timeline pluss::timeline::random(integer_range sizes,
                                 const Timeline& low,
                                 const Timeline& high,
                                 std::uint64_t seed = 0)
```

## Sample Exact Instants From A Half-Open Interval

`random()` allocates a Timeline of `sizes` and independently samples every lane
from the half-open interval `[low, high)`. The lower bound can occur; the upper
bound cannot. Half-open boundaries compose cleanly: adjacent intervals
`[a, b)` and `[b, c)` neither overlap nor leave a gap.

Sampling operates directly on signed 64-bit epoch nanoseconds. It does not
convert a broad time span through floating point, so neighboring nanoseconds
remain distinct even when the endpoints are years apart. The output owns fresh,
contiguous storage and carries the lower bound's timezone metadata.

Use [`rand_between`](rand_between.md) when the upper endpoint must be eligible
or equal bounds should produce a constant result. `random()` instead requires
`high > low` because `[x, x)` is empty.

## Bounds And Shape

Both bounds must be defined scalar Timelines. Ordering is based on stored epoch
nanoseconds, not displayed wall-clock fields. `sizes` may be empty for a scalar
result or contain up to three non-negative extents. A zero extent is valid and
produces an empty Timeline with the requested shape.

This factory accepts Timeline bounds only. That makes parsing and timezone
projection explicit at the call site: use `date()`, `time()`, `make()`, or
`from_epoch()` to build the exact interval you intend.

## Seeds

A nonzero seed reproduces the same sequence for the same inputs and build.
Seed `0` draws from the library seed stream and is intentionally not a request
for a permanently repeated sequence. Tests and published examples should pass
a nonzero seed.

The contract standardizes the interval and repeatability properties, not the
particular random-engine sequence as a file format. Persist generated ticks if
they must survive implementation upgrades exactly.

## Example 1: A Scalar Shape And One-Tick Interval

```cpp
const auto scalar = pluss::timeline::random(
    {}, pluss::timeline::from_epoch(0LL),
    pluss::timeline::from_epoch(1LL), 17);
std::cout << scalar.epoch_nanos() << '\n';
```

### Output

```text
Series(shape=[], strides=[], dtype=int64, platform=cpu, numel=1, contiguous=true)
data=0
```

Only tick `0` belongs to `[0, 1)`, which makes both the scalar shape and the
excluded upper endpoint visible without relying on probability.

## Example 2: A Seeded Matrix

```cpp
const auto matrix = pluss::timeline::random(
    {2, 3}, pluss::timeline::from_epoch(10LL),
    pluss::timeline::from_epoch(15LL), 7);
std::cout << matrix.epoch_nanos() << '\n';
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 12 14 11 ⎤
⎣ 11 14 11 ⎦
```

Every lane is at least `10` and less than `15`; no lane can equal `15`.

## Example 3: Test The Contract Rather Than One Sequence

```cpp
const auto matrix = pluss::timeline::random(
    {2, 3}, pluss::timeline::from_epoch(10LL),
    pluss::timeline::from_epoch(15LL), 7);
const auto repeated = pluss::timeline::random(
    {2, 3}, pluss::timeline::from_epoch(10LL),
    pluss::timeline::from_epoch(15LL), 7);

std::cout << (matrix.ge(pluss::timeline::from_epoch(10LL)).all().item<bool>() &&
              matrix.lt(pluss::timeline::from_epoch(15LL)).all().item<bool>())
          << '\n';
std::cout << matrix.epoch_nanos().equal(repeated.epoch_nanos()) << '\n';
```

### Output

```text
true
true
```

## Example 4: Retain The Lower Bound's Display Zone

```cpp
const auto low = pluss::timeline::make("2024-07-01T06:00:00Z")
                     .tz_convert("America/Edmonton");
const auto high = pluss::timeline::make("2024-07-01T06:00:00.000000003Z")
                      .tz_convert("America/Edmonton");
const auto local = pluss::timeline::random({3}, low, high, 5);

std::cout << pluss::note::print(
    pluss::timeline::format(local, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["2024-07-01T00:00:00.000000002-06:00", "2024-07-01T00:00:00.000000000-06:00", "2024-07-01T00:00:00.000000000-06:00"]
```

## Example 5: Empty Geometry Is Still Valid Geometry

```cpp
const auto empty = pluss::timeline::random(
    {2, 0}, pluss::timeline::from_epoch(0LL),
    pluss::timeline::from_epoch(1LL), 9);
std::cout << pluss::timeline::print(empty) << '\n';
```

### Output

```text
Timeline(shape=[2, 0], tz=UTC)[  ]
[  ]
```

## Failures And Cost

The call throws for equal or reversed bounds, absent bounds, non-scalar bounds,
negative extents, rank above three, or shape-product overflow. Validation occurs
before output values are produced. Runtime and storage are linear in the output
`numel()`.

## See Also

- [`rand_between`](rand_between.md): closed-interval sampling.
- [`from_epoch`](from_epoch.md): exact integer bounds.
- [`full`](full.md): deterministic allocation from one scalar instant.
