# TimelineIndexAccessor::tail

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
TimelineIndexAccessor TimelineIndexAccessor::tail(std::int64_t n = 5) const
```

## What It Does

Returns the last `n` timestamps.

This is the back-of-axis convenience surface.

## Parameters

**n**

How many timestamps to keep from the back.

## Returns

`TimelineIndexAccessor`
    The tail slice.

## Examples

```cpp
auto out = base.tail(2);
std::cout << pluss::timeindex::print(out) << "\n";
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-11-01 01:00:00, 2024-11-01 01:20:00 ]
```

Example 2. Use the default when you only need a quick trailing preview.

```cpp
auto preview = base.tail();
std::cout << pluss::timeindex::print(preview) << "\n";
```

## Notes

- `head()` and `tail()` are shortcuts for common positional slices.

## See Also

- [`TimelineIndexAccessor::head`](head.md)
- [`TimelineIndexAccessor::slice_index`](slice_index.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
