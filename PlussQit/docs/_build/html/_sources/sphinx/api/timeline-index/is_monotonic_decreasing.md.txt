<!-- pluss-hand-authored -->

# Timeline::TimelineIndexAccessor::is_monotonic_decreasing

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
bool Timeline::TimelineIndexAccessor::is_monotonic_decreasing() const
```

## What The Predicate Proves

`is_monotonic_decreasing()` answers whether every timestamp is less than or
equal to the timestamp before it. Equality is intentional: repeated instants do
not break monotonicity. The predicate tests order only; use
[`is_unique()`](is_unique.md) when repeated instants are the concern.

This distinction matters when validating an externally supplied axis. A series
of readings may be newest-first and still be well ordered, even though most
ordered lookup and resampling operations require the opposite, increasing
direction. This method lets a caller distinguish that deliberate reverse order
from a genuinely scrambled axis without sorting or allocating another
`Timeline::TimelineIndexAccessor`.

The result is a native C++ `bool`, not a one-element container. Empty and
single-element axes return `true`: neither contains an adjacent pair that
violates decreasing order. The check compares stored epoch nanoseconds, so the
timezone used to display those instants cannot change the answer.

## Examples

Example 1. An ordinary increasing axis is not decreasing.

```cpp
auto index = pluss::timeline::make({
    "2024-11-01T00:00:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:40:00Z",
}).index();

std::cout << std::boolalpha
          << index.is_monotonic_decreasing() << "\n";
```

### Output

```text
false
```

Example 2. Build a newest-first axis with construction-time order enforcement
disabled, then validate its intended direction explicitly.

```cpp
auto newest_first = pluss::timeline::make({
    "2024-11-01T01:20:00Z",
    "2024-11-01T01:00:00Z",
    "2024-11-01T00:40:00Z",
    "2024-11-01T00:20:00Z",
    "2024-11-01T00:00:00Z",
}).index(false);

std::cout << std::boolalpha
          << newest_first.is_monotonic_decreasing() << "\n";
```

### Output

```text
true
```

Example 3. Equal neighbors satisfy both monotonic directions. They affect
uniqueness, not ordering.

```cpp
auto tied = pluss::timeline::make({
    "2025-01-03T00:00:00Z",
    "2025-01-02T00:00:00Z",
    "2025-01-02T00:00:00Z",
    "2025-01-01T00:00:00Z",
}).index(false);

std::cout << std::boolalpha
          << "decreasing=" << tied.is_monotonic_decreasing() << "\n"
          << "unique=" << tied.is_unique() << "\n";
```

### Output

```text
decreasing=true
unique=false
```

Example 4. Trivial axes are monotonically decreasing by definition.

```cpp
auto one = pluss::timeline::make({"2025-01-01T00:00:00Z"}).index();
auto none = pluss::timeline::epoch(std::vector<std::int64_t>{}).index();

std::cout << std::boolalpha
          << "one=" << one.is_monotonic_decreasing() << "\n"
          << "none=" << none.is_monotonic_decreasing() << "\n";
```

### Output

```text
one=true
none=true
```

## Cost And Ownership

The method performs one forward scan and stops at the first increasing pair.
Its worst-case cost is linear in the number of timestamps, its auxiliary space
is constant, and it neither mutates nor copies the axis.

## See Also

- [`Timeline::TimelineIndexAccessor::is_monotonic_increasing`](is_monotonic_increasing.md): validate
  the order required by ordered lookup and resampling.
- [`Timeline::TimelineIndexAccessor::is_unique`](is_unique.md): detect repeated instants independently
  of direction.
- [`Timeline::TimelineIndexAccessor::sort`](sort.md): produce an increasing axis when reordering is
  appropriate.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersection.html">
    <span class="label">Previous</span>
    <span class="title">intersection</span>
  </a>
  <a class="next-link" href="is_monotonic_increasing.html">
    <span class="label">Next</span>
    <span class="title">is_monotonic_increasing</span>
  </a>
</div>
