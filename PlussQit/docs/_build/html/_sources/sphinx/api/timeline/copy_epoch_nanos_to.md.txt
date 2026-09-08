<!-- pluss-hand-authored -->

# Timeline::copy_ticks

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
template <std::output_iterator<std::int64_t> OutIt>
void Timeline::copy_ticks(OutIt out) const
```

## Copy Into A Caller-Chosen Destination

`copy_ticks()` writes exact UTC epoch-nanosecond ticks through a C++
output iterator. It is the allocation-policy-neutral export: the caller may
append to a vector, fill preallocated memory, or feed another iterator-based
component without first accepting the container chosen by an API.

Traversal is logical row-major order and therefore works for contiguous and
strided Timelines alike. The method flattens the traversal into the destination;
it does not emit shape or timezone metadata. Preserve `sizes()` and `tz()`
separately when the receiver must later reconstruct both.

## Example: Append Exact Ticks

```cpp
#include <pluss/pluss.h>

#include <iostream>
#include <iterator>
#include <vector>

int main() {
  const auto times = pluss::timeline::from_epoch(
      {{-1LL, 0LL}, {1'999'999'999LL, pluss::Timeline::kNaT}});

  std::vector<std::int64_t> ticks;
  times.copy_ticks(std::back_inserter(ticks));

  std::cout << '[';
  for (std::size_t i = 0; i < ticks.size(); ++i) {
    if (i) std::cout << ", ";
    std::cout << ticks[i];
  }
  std::cout << "]\n";
}
```

### Output

```text
[-1, 0, 1999999999, -9223372036854775808]
```

No position is suppressed: `NaT` is copied as `Timeline::kNaT`, and the matrix
is visited left-to-right across each row.

## Choosing The Destination

With `std::back_inserter`, reserve `numel()` elements when allocation behavior
matters:

```cpp
std::vector<std::int64_t> ticks;
ticks.reserve(static_cast<std::size_t>(times.numel()));
times.copy_ticks(std::back_inserter(ticks));
```

For preallocated storage, the range must have room for exactly `numel()`
writes:

```cpp
std::vector<std::int64_t> ticks(
    static_cast<std::size_t>(times.numel()));
times.copy_ticks(ticks.begin());
```

The output-iterator interface cannot inspect the destination capacity. Passing
an iterator without sufficient writable space is a caller error.

## Guarantees And Boundaries

- Values are copied exactly; no unit conversion or timezone conversion occurs.
- An empty Timeline performs no writes.
- A strided source is read in logical order, not raw allocation order.
- The destination receives neither rank nor shape information.
- The operation throws if the Timeline is undefined or its tick storage is
  invalid.

Use [`ticks()`](ticks.md) for the convenient
owning-vector form. Use [`epoch_nanos()`](epoch_nanos.md) when shape-preserving
numeric computation is the goal.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="concat.html"><span class="label">Previous</span><span class="title">concat</span></a>
  <a class="next-link" href="epoch_nanos.html"><span class="label">Next</span><span class="title">epoch_nanos</span></a>
</div>
