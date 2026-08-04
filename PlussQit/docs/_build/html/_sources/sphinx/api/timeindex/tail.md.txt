# TimeIndex::tail

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signature

```cpp
TimeIndex::tail(std::int64_t n = 5) const -> TimeIndex
```

## What It Does

Returns the last `n` timestamps.

This is the back-of-axis convenience surface.

## Parameters

**n**

How many timestamps to keep from the back.

## Returns

`TimeIndex`
    The tail slice.

## Examples

```cpp
auto out = base.tail(2);
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-11-01 01:00:00, 2024-11-01 01:20:00 ]
```

Example 2. Use the default when you only need a quick trailing preview.

```cpp
auto preview = base.tail();
std::cout << pluss::print_timeline(preview) << "\n";
```

## Notes

- `head()` and `tail()` are shortcuts for common positional slices.

## See Also

- [`TimeIndex::head`](head.md)
- [`TimeIndex::slice_index`](slice_index.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="union_.html">
    <span class="label">Next</span>
    <span class="title">union_</span>
  </a>
</div>
