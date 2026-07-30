<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c3839864dc4c -->

# Thread::slice

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::slice(std::int64_t start, std::int64_t stop) const -> Thread
```

## What It Does

Returns a contiguous `[start, stop)` slice as a plain `Thread` (not a `ThreadView` —
compare [`view()`](view.md), which wraps this exact operation in a `ThreadView`).
Python-slice semantics throughout: negative `start`/`stop` count from the end, both
are clamped into `[0, size()]`, and a `stop` at or before the resolved `start`
produces an empty (not erroring) result. Internally implemented as
`take({start, start+1, ..., stop-1})`.

## Parameters

**start** (`std::int64_t`)
    Inclusive starting position. Negative values count from the end.

**stop** (`std::int64_t`)
    Exclusive ending position. Negative values count from the end.

## Returns

`Thread`
    A new thread over positions `[start, stop)`, clamped into range.

## Examples

Example 1. A plain forward slice.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto out = scores.slice(1, 4);
std::cout << out;
```

### Output

```text
0  20
1  30
2  40

ptype: Series
```

Example 2. Negative indices count from the end.

```cpp
auto out2 = scores.slice(-2, 5);
std::cout << out2;
```

### Output

```text
0  40
1  50

ptype: Series
```

Example 3. An inverted range (`stop <= start` after resolving negatives) is an
empty result, not an error.

```cpp
auto out3 = scores.slice(4, 1);
std::cout << out3;
```

### Output

```text
ptype: Series
```

## Notes

- Negative `start`/`stop` count from the end (Python-slice convention).
- Both bounds are clamped into `[0, size()]` — never throws for out-of-range
  values.
- An inverted or empty resulting range produces a valid, zero-length `Thread`.

## See Also

- [`Thread::view`](view.md): the same operation, wrapped in a `ThreadView`.
- [`Thread::take`](take.md): the arbitrary-index generalization this method
  delegates to.
- [`Thread::head`](head.md) / [`Thread::tail`](tail.md): anchored convenience
  forms.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="skew.html">
    <span class="label">Previous</span>
    <span class="title">skew</span>
  </a>
  <a class="next-link" href="split.html">
    <span class="label">Next</span>
    <span class="title">split</span>
  </a>
</div>
