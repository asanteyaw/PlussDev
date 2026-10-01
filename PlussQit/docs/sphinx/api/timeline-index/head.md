# TimelineIndexAccessor::head

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
TimelineIndexAccessor TimelineIndexAccessor::head(std::int64_t n = 5) const
```

## What It Does

Returns the first `n` timestamps.

This is the front-of-axis convenience surface, not a new time-aware concept.

## Parameters

**n**

How many timestamps to keep from the front.

## Returns

`TimelineIndexAccessor`
    The front slice.

## Examples

```cpp
auto out = base.head(3);
std::cout << pluss::timeindex::print(out) << "\n";
```

### Output

```text
Timeline(shape=[3], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:20:00, 2024-11-01 00:40:00 ]
```

Example 2. Use the default when you only want a small front preview.

```cpp
auto preview = base.head();
std::cout << pluss::timeindex::print(preview) << "\n";
```

## See Also

- [`TimelineIndexAccessor::tail`](tail.md)
- [`TimelineIndexAccessor::slice_index`](slice_index.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="freq.html">
    <span class="label">Previous</span>
    <span class="title">freq</span>
  </a>
  <a class="next-link" href="host.html">
    <span class="label">Next</span>
    <span class="title">host</span>
  </a>
</div>
