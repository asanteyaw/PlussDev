<!-- pluss-hand-authored -->

# Timeline::host

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::HostAny Timeline::host() const
```

## What It Does

`host()` materializes the timeline as owning host-side epoch nanoseconds and
packages the result according to its cardinality and rank. It is intended for
C++ integration points that need ordinary values rather than temporal methods:
serializers, database bindings, custom kernels, and foreign-function bridges.

The returned `HostAny` has three possible payloads:

| Source | Predicate | Accessor | Payload |
|---|---:|---|---|
| any one-position timeline | `is_scalar()` | `scalar()` | one `std::int64_t` |
| rank 1 with zero or multiple positions | `is_vector()` | `vector()` | `std::vector<std::int64_t>` |
| rank 2 or 3 with zero or multiple positions | `is_array()` | `array()` | data, shape, timezone, contiguity |

The first row is intentionally based on **cardinality**, not rank. Consequently
a shape `[1]` timeline produces a scalar host payload even though
`Timeline::is_scalar()` is false. Check the `HostAny` predicate before opening
its payload rather than predicting the variant from the source rank.

## Epoch Representation

Every exported integer is nanoseconds since the Unix epoch in UTC. Timezone
metadata accompanies the payload, but it does not alter those integers.
`NaT` is represented by `Timeline::kNaT`, the minimum signed 64-bit value.

## Returns

An owning `Timeline::HostAny`. Its data remains valid independently of the
source timeline.

## Examples

Example 1. Open scalar and vector payloads after checking their kinds.

```cpp
auto scalar = pluss::timeline::make("2024-02-29T12:30:00Z");
auto vector = pluss::timeline::make({
    "2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"
});
auto scalar_host = scalar.host();
auto vector_host = vector.host();

std::cout << std::boolalpha
          << "scalar host: scalar=" << scalar_host.is_scalar()
          << ", value=" << scalar_host.scalar()
          << ", tz=" << scalar_host.tz() << "\n"
          << "vector host: vector=" << vector_host.is_vector()
          << ", values=[" << vector_host.vector()[0]
          << ',' << vector_host.vector()[1] << ']'
          << ", tz=" << vector_host.tz() << "\n";
```

### Output

```text
scalar host: scalar=true, value=1709209800000000000, tz=UTC
vector host: vector=true, values=[1709209800000000000,1709251200000000000], tz=UTC
```

Example 2. A higher-rank payload carries shape and flat row-major data.

```cpp
auto matrix = pluss::timeline::make({
    {"2024-02-29T12:30:00Z", "2024-03-01T00:00:00Z"},
    {"2024-03-02T06:15:00Z", ""},
});
const auto host = matrix.host();
const auto& array = host.array();

std::cout << std::boolalpha
          << "array=" << host.is_array()
          << ", sizes=[" << array.sizes[0] << ',' << array.sizes[1] << ']'
          << ", values=[";
for (std::size_t i = 0; i < array.data.size(); ++i) {
    if (i != 0) std::cout << ',';
    std::cout << array.data[i];
}
std::cout << "]"
          << ", contiguous=" << array.contiguous
          << ", tz=" << array.tz << "\n";
```

### Output

```text
array=true, sizes=[2,2], values=[1709209800000000000,1709251200000000000,1709360100000000000,-9223372036854775808], contiguous=true, tz=UTC
```

The fourth position was constructed from an empty timestamp string and is
therefore exported as the exact `NaT` sentinel.

Example 3. Distinguish source shape from returned payload kind.

```cpp
auto singleton = pluss::timeline::make({"2024-02-29T12:30:00Z"});
auto host = singleton.host();
std::cout << std::boolalpha
          << "dim=" << singleton.dim()
          << ", is_scalar=" << singleton.is_scalar()
          << ", host scalar=" << host.is_scalar() << "\n";
```

### Output

```text
dim=1, is_scalar=false, host scalar=true
```

## Gotchas And Failure Modes

- Calling `scalar()`, `vector()`, or `array()` on the wrong variant throws.
- `host()` allocates and materializes an owning copy. Do not use it merely to
  inspect rank, count, or timezone metadata.
- Array data is flat row-major logical order; use `array.sizes` to recover
  coordinates.
- A one-element vector loses its rank in this convenience representation. Use
  [`epoch_nanos()`](epoch_nanos.md) when preserving shape is required.

## See Also

- [`Timeline::ticks`](ticks.md)
- [`Timeline::epoch_nanos`](epoch_nanos.md)
- [`Timeline::is_scalar`](is_scalar.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz.html"><span class="label">Previous</span><span class="title">tz</span></a>
  <a class="next-link" href="from_buffer.html"><span class="label">Next</span><span class="title">from_buffer</span></a>
</div>
