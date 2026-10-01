<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c57ca53c7464 -->

# Stream::to_thread

[Back to Stream API Reference](../../stream_api_reference)

## Signatures

```cpp
Stream::to_thread() const -> pluss::table::Thread
Stream::to_thread(std::int64_t start, std::int64_t stop) const -> pluss::table::Thread
```

## What It Does

Converts the stream to a `pluss::table::Thread` — the single-column building block
`Stream` itself wraps a payload similarly to, and the type every `DataFrame` column is made
of. The no-argument overload converts the whole stream; the two-argument overload converts
only the half-open row range `[start, stop)`. This is the explicit bridge back to the table
stack when you need `Thread`-specific operations not exposed on `Stream` directly.

## Parameters

**start** (`std::int64_t`, range overload)
    Inclusive starting row index of the slice.

**stop** (`std::int64_t`, range overload)
    Exclusive ending row index of the slice.

## Returns

`pluss::table::Thread`
    A `Thread` wrapping the same payload (or the requested row range of it).

## Examples

Example 1. Convert an entire stream to a `Thread`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}));

std::cout << stream.to_thread();
```

### Output

```text
0  1
1  2
2  3
3  4
4  5
ptype: Series
```

Example 2. Convert only rows `[1, 4)` — a half-open range, same convention as
[`slice()`](slice.md).

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}));

std::cout << stream.to_thread(1, 4);
```

### Output

```text
0  2
1  3
2  4
ptype: Series
```

## Notes

- `stop` is exclusive — `to_thread(1, 4)` returns 3 rows (indices 1, 2, 3), not 4.
- Requires a defined stream; throws `"Stream: undefined handle"` on a default-constructed
  `Stream()`.
- The result `Thread`'s own row labels reset to a plain `0..n-1` positional index in the
  printed output, independent of any label the source `Stream` carried via
  [`lbl()`](lbl.md).

## See Also

- [`Stream::from_thread`](from_thread.md): the inverse conversion, from a `Thread` back to
  a `Stream`.
- [`Stream::slice`](slice.md): the `Stream`-native row-range operation, returning a
  `Stream` instead of converting to `Thread`.
- [`Stream::as_series`](as_series.md) / [`Stream::as_note`](as_note.md) /
  [`Stream::as_timeline`](as_timeline.md): a
  narrower escape hatch to the raw typed payload instead of a `Thread`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
