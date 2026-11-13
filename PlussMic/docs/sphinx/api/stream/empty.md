<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 879f78c5f493 -->

# Stream::empty

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
bool Stream::empty() const
```

## What It Does

Reports whether the stream has zero rows — shorthand for `size() == 0`. A distinct check
from [`defined()`](defined.md): a stream can be defined (has a real, valid payload) and
still be empty (zero rows in that payload).

## Parameters

`None`

## Returns

`bool`
    `true` if the stream has zero rows; `false` otherwise.

## Examples

Example 1. A non-empty stream.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.empty();
```

### Output

```text
0
```

Example 2. A defined stream built from an empty `Series` is empty, but still defined.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series(std::vector<double>{}));

std::cout << "defined: " << stream.defined() << "\n";
std::cout << "empty: " << stream.empty() << "\n";
std::cout << "size: " << stream.size();
```

### Output

```text
defined: 1
empty: 1
size: 0
```

## Notes

- Requires a defined stream ([`defined()`](defined.md)); throws `"Stream: undefined
  handle"` on a default-constructed `Stream()`, same as [`size()`](size.md).
- Distinct from `defined()` — see Example 2: empty and defined are independent axes, not
  opposites.

## See Also

- [`Stream::size`](size.md) / [`Stream::nrecords`](nrecords.md): the row count this method
  checks against zero.
- [`Stream::defined`](defined.md): whether the stream has a payload at all, independent of
  row count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dtype.html">
    <span class="label">Previous</span>
    <span class="title">dtype</span>
  </a>
  <a class="next-link" href="exp.html">
    <span class="label">Next</span>
    <span class="title">exp</span>
  </a>
</div>
