<!-- pluss-hand-authored -->

# Thread::shape

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::shape() const -> std::pair<std::int64_t, std::int64_t>
```

## What It Does

Returns `{size(), 1}` — a `DataFrame`-shaped view of a `Thread`'s dimensions, where a
thread is always exactly one "column" wide. This exists purely for API symmetry with
`DataFrame::shape()` (which returns `{nrows, nfeatures}`); the second element is
always `1` and carries no information `size()` doesn't already give directly.

## Parameters

`None`

## Returns

`std::pair<std::int64_t, std::int64_t>`
    `{size(), 1}`.

## Examples

Example 1. `shape()`'s second element is always `1`, regardless of length or kind.

```cpp
using namespace pluss::table;

auto numbers = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto [rows, cols] = numbers.shape();
std::cout << rows << " x " << cols;
```

### Output

```text
3 x 1
```

## Notes

- The second element is always `1` — a `Thread` never has more than one "column."
- Equivalent to `{size(), 1}` — no extra computation.

## See Also

- [`Thread::size`](size.md) / [`Thread::ncases`](ncases.md): the row count alone,
  without the fixed `1`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="shares_payload_with.html">
    <span class="label">Next</span>
    <span class="title">shares_payload_with</span>
  </a>
</div>
