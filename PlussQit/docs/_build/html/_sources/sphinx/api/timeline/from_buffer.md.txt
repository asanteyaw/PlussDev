<!-- pluss-hand-authored -->

# Timeline::from_buffer

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
static Timeline Timeline::from_buffer(
    std::int64_t* data,
    std::span<const std::int64_t> sizes,
    std::span<const std::int64_t> strides,
    pluss::BlobDeleter deleter = {},
    std::string_view tz_name = "UTC")
```

## Share Existing Storage With A Timeline

`from_buffer()` places Timeline semantics over an existing buffer
of signed 64-bit UTC epoch-nanosecond ticks. It does not copy those ticks. The
shape and element strides describe how logical positions map onto the buffer,
while `tz_name` controls calendar interpretation and rendering.

This is the lowest-level Timeline constructor. Use it when another C++
component already owns a suitably aligned tick buffer, when a strided view must
remain strided, or when copying a large temporal array would be wasteful. Use
[`pluss::timeline::from_epoch()`](epoch_nanos.md) when values should instead be
copied into storage managed entirely by Pluss.

Strides are measured in **elements**, not bytes. For `sizes={2}` and
`strides={2}`, logical positions read `data[0]` and `data[2]`; `data[1]` is not
part of the Timeline.

## Ownership And Mutation

An empty `deleter` creates borrowed storage. The caller must keep the buffer
alive, aligned, and unchanged in size until the Timeline and every copy or view
derived from it have been destroyed.

A non-empty `deleter` transfers lifetime responsibility to shared Timeline
storage. It is called once, with the original pointer, after the final owner is
released. Copying the Timeline shares that ownership; it does not invoke the
deleter early.

Because construction is zero-copy, writes through the original pointer are
observable through the Timeline. Coordinate concurrent access yourself. The
owning exports [`epoch_nanos()`](epoch_nanos.md),
[`ticks()`](ticks.md), and
[`copy_ticks()`](copy_ticks.md) take snapshots instead.

## Example: Own A Strided Allocation

```cpp
#include <pluss/pluss.h>

#include <array>
#include <cstdlib>
#include <iostream>

int main() {
  bool released = false;
  {
    auto* raw = static_cast<std::int64_t*>(
        std::malloc(3 * sizeof(std::int64_t)));
    raw[0] = 0;
    raw[1] = 111;                 // padding, not a logical position
    raw[2] = 2'000'000'000LL;

    const std::array<std::int64_t, 1> sizes{2};
    const std::array<std::int64_t, 1> strides{2};
    const auto times = pluss::Timeline::from_buffer(
        raw, sizes, strides,
        [&](void* p) {
          released = true;
          std::free(p);
        },
        "UTC");

    std::cout << pluss::timeline::print(times) << '\n';
    std::cout << "released while Timeline lives: "
              << std::boolalpha << released << '\n';
  }
  std::cout << "released after Timeline dies: "
            << std::boolalpha << released << '\n';
}
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 1970-01-01 00:00:00, 1970-01-01 00:00:02 ]
released while Timeline lives: false
released after Timeline dies: true
```

The output proves both contracts: logical traversal honors stride `2`, and the
callback runs only after the Timeline releases the allocation.

## Contract And Failure Modes

- `data` must address storage suitable for `std::int64_t` values.
- `sizes` and `strides` must describe the same rank; Timeline supports ranks
  zero through three.
- Every reachable offset must lie inside the allocation. The constructor
  cannot infer the allocation's byte length and cannot prove that condition.
- `Timeline::kNaT` is interpreted as the absent-instant sentinel. Every other
  bit pattern is an epoch-nanosecond tick.
- `tz_name` does not reinterpret or shift the integers. It supplies the zone in
  which the same instants are rendered and decomposed into calendar fields.
- A dangling borrowed buffer, an incorrect stride, or a deleter that does not
  match the allocation mechanism is a caller error with undefined behavior.

## See Also

- [`Timeline::ticks`](ticks.md)
- [`Timeline::copy_ticks`](copy_ticks.md)
- [`Timeline::epoch_nanos`](epoch_nanos.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="host.html"><span class="label">Previous</span><span class="title">host</span></a>
  <a class="next-link" href="ticks.html"><span class="label">Next</span><span class="title">ticks</span></a>
</div>
