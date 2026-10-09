# Timeline::index

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::TimelineIndexAccessor Timeline::index(bool require_sorted = true) const
```

## Treat Temporal Values As An Axis

`index()` is the public entrance to Timeline's ordered-axis operations. A
Timeline normally represents temporal values: it may be scalar,
multidimensional, unordered, or contain `NaT`. An index has a narrower job. It
must be a one-dimensional sequence of present instants, and it is
nondecreasing unless the caller explicitly requests an unchecked accessor.

Open the dedicated
[Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference.md)
for the complete ordered-axis method index and the individual reference page
for every accessor operation.

The returned object owns a cheap Timeline handle. Construct it once and retain
it when several index operations belong to the same workflow:

```cpp
const auto index = timestamps.index();

const auto insertion = index.searchsorted(key);
const auto previous = index.asof(key);
const auto bins = index.resample_bins("D");
```

The monotonic scan occurs at accessor construction. The later calls reuse that
validated state. Ordered lookup, as-of matching, alignment, rolling windows,
and resampling describe how temporal values are being used; they do not create
another temporal dtype.

## Parameter

| Parameter | Meaning | Default |
| --- | --- | --- |
| `require_sorted` | Validate nondecreasing epoch order while building the accessor | `true` |

Passing `false` skips only the ordering test. Rank-one and present-value
requirements remain mandatory. It neither sorts nor deduplicates the receiver.

## Validation Contract

| Property | Contract |
| --- | --- |
| Rank | Exactly one; scalars and matrices are rejected |
| Length | Any nonnegative length, including zero |
| Values | Every lane is a defined instant; `NaT` is rejected |
| Ordering | Nondecreasing by default; adjacent duplicates are valid |
| Shape | The original vector shape `[n]` is retained |
| Timezone | Retained exactly |
| Precision | Signed 64-bit epoch nanoseconds are retained exactly |

Ordering and uniqueness are independent. Duplicate timestamps satisfy the
default contract because `previous <= current`; call `is_unique()` when an
algorithm requires one key per instant.

## Example: Reuse One Validated Accessor

```cpp
const auto times = pluss::timeline::make({
    "2025-01-01T09:00:00Z",
    "2025-01-01T12:00:00Z",
    "2025-01-02T09:00:00Z",
});
const auto index = times.index();
const auto key = pluss::timeline::make("2025-01-01T10:00:00Z");

std::cout << "insert=" << index.searchsorted(key)
          << " asof=" << index.asof(key)
          << " increasing=" << std::boolalpha
          << index.is_monotonic_increasing() << '\n';
```

### Output

```text
insert=1 asof=0 increasing=true
```

The three operations use one checked axis. Repeatedly constructing temporary
accessors would repeat the validation work.

## Example: Duplicates And Empty Axes

```cpp
const auto duplicates = pluss::timeline::make({
    "2025-01-01T00:00:00Z",
    "2025-01-01T00:00:00Z",
    "2025-01-03T00:00:00Z",
}).index();
const auto empty = pluss::timeline::epoch({0}).index();

std::cout << "duplicate_size=" << duplicates.size()
          << " unique=" << std::boolalpha << duplicates.is_unique() << '\n';
std::cout << "empty_size=" << empty.size()
          << " increasing=" << empty.is_monotonic_increasing() << '\n';
```

### Output

```text
duplicate_size=3 unique=false
empty_size=0 increasing=true
```

An empty vector is ordered vacuously. The `{0}` argument supplies a rank-one
shape; a scalar Timeline has shape `[]` and is rejected.

## Example: Inspect An Unordered Axis

```cpp
const auto times = pluss::timeline::make({
    "2025-01-03T00:00:00Z",
    "2025-01-01T00:00:00Z",
});

try {
  (void)times.index();
} catch (const pluss::Error&) {
  std::cout << "checked=rejected\n";
}

const auto unchecked = times.index(false);
std::cout << "unchecked_increasing=" << std::boolalpha
          << unchecked.is_monotonic_increasing() << '\n';
```

### Output

```text
checked=rejected
unchecked_increasing=false
```

The unchecked accessor preserves the original order. Binary-search operations
do not become meaningful merely because validation was skipped. Prefer
`times.sort().index()` when ordered lookup is the goal.

## Example: Borrowed Storage And Timezone Survive

```cpp
auto* ticks = static_cast<std::int64_t*>(
    std::malloc(3 * sizeof(std::int64_t)));
ticks[0] = 0;
ticks[1] = 1'000'000'000LL;
ticks[2] = 2'000'000'000LL;

const std::array<std::int64_t, 1> shape{3};
const std::array<std::int64_t, 1> strides{1};
const auto times = pluss::Timeline::from_buffer(
    ticks, shape, strides, [](void* p) { std::free(p); },
    "America/Edmonton");
const auto index = times.index();

ticks[1] = 1'500'000'000LL;
std::cout << "tz=" << index.timeline().tz()
          << " middle=" << index.host()[1] << '\n';
```

### Output

```text
tz=America/Edmonton middle=1500000000
```

Packed borrowed storage remains shared. Callers therefore must not mutate an
external buffer in a way that invalidates ordering while a checked accessor is
in use. A permanent Timeline-level monotonicity cache would be unsafe until
borrowed storage is immutable or version-aware.

## Failure Modes

`index()` throws when the receiver is scalar, has rank greater than one,
contains `NaT`, or violates the requested ordering contract. Passing `false`
relaxes only the final condition. No path silently reshapes, sorts, removes
duplicates, or substitutes absent instants.

## See Also

- [`Timeline::sort`](sort.md): establish nondecreasing order before indexing.
- [`Timeline::unique`](unique.md): retain the first occurrence of each instant.

<!-- pluss-hand-authored -->
