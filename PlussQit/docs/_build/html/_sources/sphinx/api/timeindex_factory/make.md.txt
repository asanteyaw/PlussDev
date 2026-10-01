<!-- pluss-hand-authored -->

# pluss::timeindex::make

[Back to TimeIndex Factory API Reference](../../timeindex_factory_api_reference)

## Call Forms

```cpp
pluss::TimeIndex pluss::timeindex::make(
    const pluss::Timeline& values,
    bool require_sorted = true)

pluss::TimeIndex pluss::timeindex::make(
    timestamp_text_input values,
    bool require_sorted = true)

pluss::TimeIndex pluss::timeindex::make(
    epoch_nanosecond_input values,
    bool require_sorted = true)

pluss::TimeIndex pluss::make_timeindex(
    const pluss::Timeline& values,
    bool require_sorted = true)

pluss::TimeIndex pluss::make_timeindex(
    timestamp_text_input values,
    bool require_sorted = true)

pluss::TimeIndex pluss::make_timeindex(
    epoch_nanosecond_input values,
    bool require_sorted = true)
```

`timestamp_text_input` includes a scalar string and a contiguous one-dimensional
string range. `epoch_nanosecond_input` includes a scalar signed integer and a
contiguous one-dimensional signed-integer range. These names describe accepted
call-site concepts; no private reader type is part of the public contract.

`pluss::make_timeindex(...)` is the intentional root-level forwarding spelling.
It has the same accepted inputs and default ordering check as
`pluss::timeindex::make(...)`; choosing it changes no storage, ownership,
timezone, or validation behavior.

## Promote Values Into Index Semantics

`make()` is the canonical entrance to `TimeIndex`. Text is parsed as timestamp
instants, integers are interpreted as exact epoch nanoseconds, and an existing
`Timeline` contributes its values and timezone. Scalar text or integer input is
promoted to a one-element axis because TimeIndex is always rank one.

Construction validates the whole axis before returning it. `NaT` is rejected:
an index position must identify an instant. An input Timeline must be rank one.
With the default `require_sorted=true`, ticks must be non-decreasing; equal
timestamps are allowed, so ordering and uniqueness remain separate contracts.

Set `require_sorted=false` only when unordered input must be represented long
enough to call `sort()`, `argsort()`, or inspect it. Binary search, rolling, and
resampling operations either require or assume increasing order and should not
be applied before normalization.

## Examples

Example 1. Scalar text becomes a one-element axis rather than a rank-zero
temporal value.

```cpp
auto index = pluss::timeindex::make("2025-01-01T00:00:00Z");
std::cout << pluss::timeindex::print(index) << "\n";
```

### Output

```text
TimeIndex(shape=[1], tz=UTC)
[ 2025-01-01 00:00:00 ]
```

Example 2. A text vector supplies the ordinary ordered-axis path.

```cpp
auto index = pluss::timeindex::make({
    "2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z",
    "2025-01-01T02:00:00Z"});
std::cout << pluss::timeindex::print(index) << "\n";
```

### Output

```text
TimeIndex(shape=[3], tz=UTC)
[ 2025-01-01 00:00:00, 2025-01-01 01:00:00, 2025-01-01 02:00:00 ]
```

Example 3. Integer input preserves exact epoch-nanosecond identity.

```cpp
auto index = pluss::timeindex::make(
    std::vector<std::int64_t>{0, 1'000'000'000LL, 2'000'000'000LL});
std::cout << pluss::timeindex::print(index) << "\n";
```

### Output

```text
TimeIndex(shape=[3], tz=UTC)
[ 1970-01-01 00:00:00, 1970-01-01 00:00:01, 1970-01-01 00:00:02 ]
```

Example 4. Timeline promotion preserves timezone metadata.

```cpp
auto local = pluss::timeline::make({
    "2025-01-01T09:00:00", "2025-01-01T10:00:00"})
                 .tz_localize("America/Edmonton");
std::cout << pluss::timeindex::print(pluss::timeindex::make(local)) << "\n";
```

### Output

```text
TimeIndex(shape=[2], tz=America/Edmonton)
[ 2025-01-01 09:00:00, 2025-01-01 10:00:00 ]
```

Example 5. The root-level `make_*` spelling is exactly equivalent.

```cpp
const auto canonical = pluss::timeindex::make({
    "2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z"});
const auto convenient = pluss::make_timeindex({
    "2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z"});
std::cout << std::boolalpha
          << "make_timeindex_equal=" << (canonical.host() == convenient.host())
          << "\n";
```

### Output

```text
make_timeindex_equal=true
```

Example 6. Explicitly retain unordered input for a later ordering operation.

```cpp
auto index = pluss::timeindex::make({
    "2025-01-01T02:00:00Z", "2025-01-01T00:00:00Z",
    "2025-01-01T01:00:00Z"}, false);
std::cout << pluss::timeindex::print(index) << "\n";
```

### Output

```text
TimeIndex(shape=[3], tz=UTC)
[ 2025-01-01 02:00:00, 2025-01-01 00:00:00, 2025-01-01 01:00:00 ]
```

Example 7. Default construction rejects that same ordering.

```cpp
try {
  (void)pluss::timeindex::make({
      "2025-01-01T01:00:00Z", "2025-01-01T00:00:00Z"});
} catch (const std::exception& error) {
  std::cout << std::string_view(error.what()).substr(0,
      std::string_view(error.what()).find('\n')) << "\n";
}
```

### Output

```text
TimeIndex must be sorted (non-decreasing)
```

Example 8. A matrix Timeline cannot become a one-dimensional index.

```cpp
try {
  (void)pluss::timeindex::make(pluss::timeline::make({
      {"2025-01-01T00:00:00Z", "2025-01-01T01:00:00Z"}}));
} catch (const std::exception& error) {
  std::cout << std::string_view(error.what()).substr(0,
      std::string_view(error.what()).find('\n')) << "\n";
}
```

### Output

```text
TimeIndex requires a 1D Timeline
```

## Ownership And Cost

Text and integer call forms allocate TimeIndex storage. Timeline promotion
shares already packed tick storage through the underlying reference-counted
Series; a strided Timeline is materialized once so subsequent ordered kernels
can scan packed memory safely. Validation is `O(n)` and does not reorder input.

## See Also

- [`from_buffer`](from_buffer.md)
- [`date_range`](date_range.md)
- [`TimelineIndexAccessor::sort`](../timeline-index/sort.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_buffer.html"><span class="label">Previous</span><span class="title">from_buffer</span></a>
  <a class="next-link" href="print.html"><span class="label">Next</span><span class="title">print</span></a>
</div>
