<!-- pluss-hand-authored -->
# pluss::timeline::full_like

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signature

```cpp
Timeline pluss::timeline::full_like(const Timeline& like,
                                    const Timeline& fill_value)
```

## Separate Geometry From Value

`full_like()` allocates a new `Timeline` with the same rank and extents as
`like`, then repeats one scalar `fill_value` throughout that geometry. It is
useful when an existing temporal object establishes batch or grid shape but its
instants must not flow into the result.

Only `like.sizes()` is used. Its ticks, timezone, strides, and ordering do not
become output data. The output timezone comes from `fill_value`, because the
fill instant supplies both the repeated epoch tick and its presentation
metadata. This distinction matters when the shape source and fill use different
zones.

The result owns a fresh contiguous allocation and aliases neither input. Its
construction cost is linear in `like.numel()`. Zero-element shapes remain
meaningful and produce no fill writes.

## Example 1: Shape Is Copied But Values Are Ignored

```cpp
const auto shape_source = pluss::timeline::make({
    {"2020-01-01", "2021-01-01"},
    {"2022-01-01", "2023-01-01"},
});
const auto filled = pluss::timeline::full_like(
    shape_source, pluss::timeline::make("1999-12-31T23:00:00Z"));

std::cout << pluss::note::print(
    pluss::timeline::format(filled, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["1999-12-31T23:00:00.000000000+00:00", "1999-12-31T23:00:00.000000000+00:00"], 
  ["1999-12-31T23:00:00.000000000+00:00", "1999-12-31T23:00:00.000000000+00:00"]
]
```

None of the four years in `shape_source` appears in the result.

## Example 2: The Fill Timezone Wins

```cpp
const auto shape_source = pluss::timeline::from_epoch({0LL, 1LL}, "UTC");
const auto local_fill = pluss::timeline::from_epoch(
    1'709'208'000'123'456'789LL, "America/Edmonton");
const auto local = pluss::timeline::full_like(shape_source, local_fill);

std::cout << pluss::note::print(
    pluss::timeline::format(local, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2024-02-29T05:00:00.123456789-07:00", "2024-02-29T05:00:00.123456789-07:00"]
```

UTC belongs to the shape source, not to the generated value. The result uses
`America/Edmonton` from `local_fill`.

## Example 3: Rank-Three Shape And Exact Ticks

```cpp
const auto block = pluss::timeline::full_like(
    pluss::timeline::epoch({1, 2, 2}),
    pluss::timeline::from_epoch(7LL));
std::cout << pluss::series::print(block.epoch_nanos()) << '\n';
```

### Output

```text
Series(shape=[1,2,2], strides=[4,2,1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data[0, :, :] =
⎡ 7 7 ⎤
⎣ 7 7 ⎦
```

## Example 4: Empty Geometry Needs No Special Case

```cpp
const auto local_fill = pluss::timeline::from_epoch(
    1'709'208'000'123'456'789LL, "America/Edmonton");
const auto empty = pluss::timeline::full_like(
    pluss::timeline::epoch({0, 3}), local_fill);
std::cout << pluss::timeline::print(empty) << '\n';
```

### Output

```text
Timeline(shape=[0, 3], tz=America/Edmonton)
```

The result preserves `[0, 3]` and the fill timezone even though it contains no
instants.

## Validation And Failure

`like` may have any valid `Timeline` shape, including scalar or empty geometry.
`fill_value` must be rank zero; a one-element vector remains a vector and is
rejected. Since the shape originates from an already valid `Timeline`, no
separate shape argument can disagree with it.

```cpp
try {
  (void)pluss::timeline::full_like(
      pluss::timeline::epoch({2}), pluss::timeline::epoch({1}));
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}
```

### Output

```text
rejected
```

## See Also

- [`full`](full.md): provide the destination shape directly.
- [`epoch`](epoch.md): allocate geometry filled with tick zero.
- [`from_epoch`](from_epoch.md): construct lane-specific integer ticks.
