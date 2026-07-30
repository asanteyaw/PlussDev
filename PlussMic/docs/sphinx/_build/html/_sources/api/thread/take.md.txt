<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d5a04fa6102a -->

# Thread::take

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::take(std::span<const std::int64_t> indices) const -> Thread
```

## What It Does

Returns the entries at the given positions, in the order `indices` specifies —
positions may repeat and don't need to be sorted, so `take()` can reorder,
duplicate, or subset in one call. Every kind-specific payload path
(`Series`/`Note`/`Timeline`/`TimeIndex`) resolves indices the same way: negative
values count from the end, and an out-of-range index (after resolving negatives)
throws `"Thread: index out of bounds"`. Both [`slice()`](slice.md) and
[`filter()`](filter.md) are implemented as `take()` calls with a computed index
list.

## Parameters

**indices** (`std::span<const std::int64_t>`)
    The positions to select, in the order they should appear in the result.
    Repeats are allowed. Negative values count from the end; any index still
    out-of-range after that resolution throws.

## Returns

`Thread`
    A new thread, length `indices.size()`, holding the selected entries in the
    requested order.

## Examples

Example 1. Reorder and duplicate in one call.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0}));
std::vector<std::int64_t> picks{2, 0, 0};
auto out = scores.take(picks);
std::cout << out;
```

### Output

```text
0  30
1  10
2  10

ptype: Series
```

Example 2. Negative indices count from the end.

```cpp
std::vector<std::int64_t> picks2{-1, -2};
auto out2 = scores.take(picks2);
std::cout << out2;
```

### Output

```text
0  30
1  20

ptype: Series
```

Example 3. An out-of-range index throws.

```cpp
try {
  std::vector<std::int64_t> bad_picks{10};
  auto bad = scores.take(bad_picks);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread: index out of bounds
```

## Notes

- Order and repeats in `indices` are both preserved/allowed — `take()` is a general
  gather, not a filter.
- Negative indices count from the end; anything still out of range throws
  `"Thread: index out of bounds"`.
- [`slice()`](slice.md) and [`filter()`](filter.md) are both implemented on top of
  `take()`.

## See Also

- [`Thread::slice`](slice.md): a contiguous-range convenience built on `take()`.
- [`Thread::filter`](filter.md): a boolean-mask convenience built on `take()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="titlecase.html">
    <span class="label">Next</span>
    <span class="title">titlecase</span>
  </a>
</div>
