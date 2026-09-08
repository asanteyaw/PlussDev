<!-- pluss-hand-authored -->
# pluss::timeline::rand_between

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signatures

```cpp
Timeline pluss::timeline::rand_between(integer_range sizes,
                                       const Timeline& low,
                                       const Timeline& high,
                                       std::uint64_t seed = 0)
Timeline pluss::timeline::rand_between(std::string_view low,
                                       std::string_view high,
                                       integer_range sizes,
                                       std::uint64_t seed = 0)
```

## Sample A Closed Interval Of Instants

`rand_between()` allocates a Timeline with the requested shape and samples each
lane uniformly from the closed integer nanosecond interval `[low, high]`.
Both endpoints are eligible. This makes the factory suitable when the upper
boundary is itself a valid outcome, and it makes `low == high` a useful,
well-defined constant construction.

The distribution is over representable instants, not displayed calendar
labels. Text dates such as `"2024-01-01"` and `"2024-01-03"` delimit a
continuous interval containing every nanosecond between the two midnights;
they do not mean "choose one of three dates." For whole-day sampling, sample an
integer day offset and add `pluss::duration::days(offset)` to a fixed midnight.

Use `random()` when the upper endpoint must be excluded. Use `full()` when a
constant result is intentional rather than a degenerate random interval.

## Bounds, Shape, And Timezone

Timeline bounds must each be rank-zero, defined, and ordered so `high >= low`.
Their stored epoch nanoseconds define the interval. The result carries the
lower bound's timezone metadata, so its fields and default printing use the
same civil projection without changing sampled instants.

Text bounds must have the same kind: date with date, time with time, or
datetime with datetime. Date-only bounds use midnight; time-only bounds use the
Unix epoch date; datetime bounds retain explicit offsets while being stored as
absolute epoch nanoseconds.

`sizes` may describe a scalar or a rank-one, rank-two, or rank-three result.
Zero extents are valid and produce an empty Timeline of that shape. Negative
extents, rank above three, and element-count overflow are rejected before
allocation.

## Seeds And Reproducibility

A nonzero seed creates a repeatable sequence for the same build and inputs.
Seed `0` requests the library's evolving seed stream; it does not promise the
same values on repeated calls. Reproducible examples, tests, and simulations
should therefore pass an explicit nonzero seed.

Do not make persisted file formats depend on the exact generated sequence.
The contract is uniform closed-interval sampling and same-seed reproducibility,
not permanent standardization of a particular random-engine implementation.

## Example 1: Equal Bounds Produce A Constant Matrix

```cpp
const auto constant = pluss::timeline::rand_between(
    "2024-02-29", "2024-02-29", {2, 2}, 99);

std::cout << pluss::note::print(
    pluss::timeline::format(constant, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-02-29T00:00:00.000000000+00:00", "2024-02-29T00:00:00.000000000+00:00"], 
  ["2024-02-29T00:00:00.000000000+00:00", "2024-02-29T00:00:00.000000000+00:00"]
]
```

## Example 2: Demonstrate The Closed Nanosecond Boundary

```cpp
const auto sampled = pluss::timeline::rand_between(
    {8},
    pluss::timeline::from_epoch(100LL),
    pluss::timeline::from_epoch(102LL),
    99);

std::cout << sampled.epoch_nanos() << '\n';
```

### Output

```text
Series(shape=[8], strides=[1], dtype=int64, platform=cpu, numel=8, contiguous=true)
data=[101, 102, 100, 101, 102, 100, 101, 100]
```

Seeing `102` confirms that the upper endpoint participates. Every lane is one
of the three exactly representable ticks; no floating-point conversion is used
to select nanoseconds.

## Example 3: Verify A Seed Without Depending On Its Particular Values

```cpp
const auto a = pluss::timeline::rand_between(
    "2024-01-01", "2024-01-03", {5}, 7);
const auto b = pluss::timeline::rand_between(
    "2024-01-01", "2024-01-03", {5}, 7);

std::cout << a.epoch_nanos().equal(b.epoch_nanos()) << '\n';
```

### Output

```text
true
```

This is the stable property applications should test. Calling with seed `0`
twice is intentionally not the equivalent operation.

## Example 4: The Lower Bound Supplies The Display Zone

```cpp
const auto low = pluss::timeline::make("2024-07-01T06:00:00Z")
                     .tz_convert("America/Edmonton");
const auto high = pluss::timeline::make("2024-07-01T06:00:00.000000002Z")
                      .tz_convert("America/Edmonton");
const auto local = pluss::timeline::rand_between({3}, low, high, 5);

std::cout << pluss::note::print(
    pluss::timeline::format(local, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["2024-07-01T00:00:00.000000002-06:00", "2024-07-01T00:00:00.000000000-06:00", "2024-07-01T00:00:00.000000000-06:00"]
```

## Failure Modes

The call throws for reversed bounds, absent bounds, non-scalar Timeline bounds,
mixed text kinds, malformed endpoint text, invalid timezone text, invalid
shape, or shape-product overflow. Bounds in different display timezones are
still ordered by their absolute epoch nanoseconds; normalize display metadata
first if inheriting the lower bound's zone would surprise the reader.

Sampling allocates contiguous storage and performs one random draw per output
lane, so time and storage are both linear in `numel()`.

## See Also

- [`random`](random.md): sample the half-open interval `[low, high)`.
- [`full`](full.md): allocate a known constant instant.
- [`from_epoch`](from_epoch.md): construct exact integer nanosecond bounds.
