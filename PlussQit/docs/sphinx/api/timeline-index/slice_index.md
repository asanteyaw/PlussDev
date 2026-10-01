# TimelineIndexAccessor::slice_index

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
TimelineIndexAccessor TimelineIndexAccessor::slice_index(std::int64_t start, std::int64_t stop) const
```

## What It Does

Slices the index by positional range.

This is the integer-position counterpart to the label-based
[`slice`](slice.md) and [`slice_time`](slice_time.md) methods.

Use it when the temporal question has already been resolved into positions.

## Parameters

**start**

The first included position.

**stop**

The first excluded position.

## Returns

`TimelineIndexAccessor`
    The positional slice.

## Examples

```cpp
auto out = base.slice_index(1, 4);
std::cout << pluss::timeindex::print(out) << "\n";
```

### Output

```text
Timeline(shape=[3], tz=UTC)
[ 2024-11-01 00:20:00, 2024-11-01 00:40:00, 2024-11-01 01:00:00 ]
```

Example 2. Reach for `slice_index()` when you want positional semantics that are
independent of the actual timestamp values.

```cpp
auto middle = base.slice_index(1, 4);
std::cout << pluss::timeindex::print(middle) << "\n";
```

## Notes

- `start` is inclusive and `stop` is exclusive.
- This is the positional sibling of the time-bound methods, not a replacement
  for them.

## See Also

- [`TimelineIndexAccessor::head`](head.md)
- [`TimelineIndexAccessor::tail`](tail.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="slice_time.html">
    <span class="label">Next</span>
    <span class="title">slice_time</span>
  </a>
</div>
