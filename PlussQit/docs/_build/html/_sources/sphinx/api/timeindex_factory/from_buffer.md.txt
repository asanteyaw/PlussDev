<!-- pluss-hand-authored -->

# pluss::timeindex::from_buffer

[Back to TimeIndex Factory API Reference](../../timeindex_factory_api_reference)

## Signature

```cpp
pluss::TimeIndex pluss::timeindex::from_buffer(
    std::int64_t* data,
    std::int64_t size,
    std::int64_t stride = 1,
    pluss::BlobDeleter deleter = {},
    std::string_view tz_name = "UTC",
    bool require_sorted = true)
```

## Adopt External Epoch-Nanosecond Storage

`from_buffer()` is the explicit systems boundary for storage allocated outside
TimeIndex. Each logical lane is one signed 64-bit epoch-nanosecond tick at
`data[i * stride]`. The factory validates index invariants, records timezone
presentation metadata, and arranges for `deleter` to run exactly once when the
adopted source storage is no longer needed.

Use this path for foreign runtimes, memory-mapped regions, custom allocators,
or producer libraries that already own tick storage. Use [`make()`](make.md)
for ordinary vectors and literals; it has fewer lifetime obligations.

## Parameters And Invariants

| Parameter | Meaning | Default |
|---|---|---|
| `data` | Address of the first signed 64-bit epoch-nanosecond value | required |
| `size` | Number of logical timestamps | required |
| `stride` | Distance between logical values, measured in `int64_t` elements | `1` |
| `deleter` | Callback receiving the original allocation address | empty |
| `tz_name` | Timezone used to present the stored instants | `"UTC"` |
| `require_sorted` | Require non-decreasing ticks | `true` |

`size` must be non-negative and `stride` positive. Every logical tick must be
defined; the reserved `NaT` value is rejected. Timezone metadata never rewrites
ticks. With ordering enforcement disabled, the resulting axis may be inspected
or sorted, but ordered lookup and window operations remain inappropriate until
it is increasing.

## Sharing Versus Materialization

A packed buffer (`stride == 1`) is adopted without copying. Mutations through
the original pointer remain visible while that pointer is valid, and the
deleter runs after the final shared owner releases the storage.

A strided buffer is different. TimeIndex kernels require packed ticks, so the
factory copies logical lanes once during construction. The original strided
allocation can then be released immediately through its deleter; subsequent
TimeIndex values own the packed materialization. This distinction prevents
every search and rolling kernel from paying stride-aware access costs.

## Examples

Example 1. A packed buffer is shared and its callback is deferred.

```cpp
bool freed = false;
{
  auto* raw = static_cast<std::int64_t*>(
      std::malloc(3 * sizeof(std::int64_t)));
  raw[0] = 0;
  raw[1] = 1'000'000'000LL;
  raw[2] = 2'000'000'000LL;

  auto index = pluss::timeindex::from_buffer(
      raw, 3, 1,
      [&](void* pointer) { freed = true; std::free(pointer); });
  raw[1] = 1'500'000'000LL;

  const auto ticks = index.host();
  std::cout << "ticks=[" << ticks[0] << ", " << ticks[1]
            << ", " << ticks[2] << "]\n";
  std::cout << std::boolalpha << "freed_inside=" << freed << "\n";
}
std::cout << std::boolalpha << "freed_after=" << freed << "\n";
```

### Output

```text
ticks=[0, 1500000000, 2000000000]
freed_inside=false
freed_after=true
```

Example 2. A stride of two selects alternating storage lanes and materializes
them immediately.

```cpp
bool freed = false;
auto* raw = static_cast<std::int64_t*>(
    std::malloc(6 * sizeof(std::int64_t)));
raw[0] = 0;               raw[1] = -1;
raw[2] = 1'000'000'000LL; raw[3] = -1;
raw[4] = 2'000'000'000LL; raw[5] = -1;

auto index = pluss::timeindex::from_buffer(
    raw, 3, 2,
    [&](void* pointer) { freed = true; std::free(pointer); });
const auto ticks = index.host();
std::cout << "ticks=[" << ticks[0] << ", " << ticks[1]
          << ", " << ticks[2] << "]\n";
std::cout << std::boolalpha << "source_freed=" << freed << "\n";
```

### Output

```text
ticks=[0, 1000000000, 2000000000]
source_freed=true
```

Example 3. Timezone metadata controls presentation without changing the raw
tick.

```cpp
auto* raw = static_cast<std::int64_t*>(std::malloc(sizeof(std::int64_t)));
raw[0] = 1'735'748'400'000'000'000LL;
auto local = pluss::timeindex::from_buffer(
    raw, 1, 1, [](void* pointer) { std::free(pointer); },
    "America/Edmonton");
std::cout << pluss::timeindex::print(local) << "\n";
```

### Output

```text
TimeIndex(shape=[1], tz=America/Edmonton)
[ 2025-01-01 09:20:00 ]
```

Example 4. Invalid stride is rejected before ownership is adopted.

```cpp
std::int64_t value = 0;
try {
  (void)pluss::timeindex::from_buffer(&value, 1, 0);
} catch (const std::exception& error) {
  std::cout << std::string_view(error.what()).substr(0,
      std::string_view(error.what()).find('\n')) << "\n";
}
```

### Output

```text
timeindex::from_buffer: stride must be positive
```

## Safety And Cost

The caller must supply storage large enough for every addressed lane and must
not free it independently after successful adoption. The deleter must match
the allocator and must not throw. Packed construction is `O(n)` validation
without a data copy; strided construction performs `O(n)` validation plus one
`O(n)` packed copy.

## See Also

- [`make`](make.md)
- [`TimelineIndexAccessor::host`](../timeline-index/host.md)
- [`TimelineIndexAccessor::timeline`](../timeline-index/timeline.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="date_range.html"><span class="label">Previous</span><span class="title">date_range</span></a>
  <a class="next-link" href="make.html"><span class="label">Next</span><span class="title">make</span></a>
</div>
