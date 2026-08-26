# TimeIndex::slice_time

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signature

```cpp
TimeIndex::slice_time(std::string_view start_iso, std::string_view end_iso) const -> pluss::Series
```

## What It Does

Returns the matching positional indices for an inclusive slice specified with
ISO-like text bounds.

This is the convenience overload of [`slice`](slice.md). It is usually the
surface you want when the bounds already exist as timestamp strings in config,
user input, or glue code.

The temporal semantics stay the same as `slice()`. The only difference is that
string parsing happens inside the call instead of before it.

## Parameters

**start_iso**

Inclusive lower bound written as an ISO-like timestamp string.

Explicit timezone information is safer when correctness matters.

**end_iso**

Inclusive upper bound written as an ISO-like timestamp string.

## Returns

`pluss::Series`
    A 1D integer `Series` of matching positions.

## Examples

Example 1. Slice directly from ISO-like text bounds.

```cpp
auto rows = idx.slice_time(
    "2024-11-01T00:15:00Z",
    "2024-11-01T01:00:00Z");
std::cout << pluss::series::print(rows) << "\n";
```

### Output

```text
Series(shape=[3], strides=[1], dtype=int64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

Example 2. Use config-provided bounds without first constructing scalar
`Timeline` objects by hand.

```cpp
std::string from_cfg = "2024-11-01T00:15:00Z";
std::string to_cfg = "2024-11-01T01:00:00Z";

auto rows = idx.slice_time(from_cfg, to_cfg);
std::cout << pluss::series::print(rows) << "\n";
```

This is the most natural form when the bounds already live in metadata,
configuration, or user-facing text input.

## Notes

- `slice_time()` is functionally the same idea as [`slice`](slice.md); it just
  moves scalar parsing inside the call.
- Reach for `slice()` instead when the bounds are already `Timeline` scalars in
  the surrounding code.
- Explicit timezone suffixes such as `Z` or `-07:00` are safer than ambiguous
  local strings when correctness matters.

## See Also

- [`TimeIndex::slice`](slice.md): same semantics, but scalar `Timeline` bounds.
- [`TimeIndex::asof`](asof.md): nearest previous timestamp position instead of a
  full range.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="asof.html">
    <span class="label">Next</span>
    <span class="title">asof</span>
  </a>
</div>
