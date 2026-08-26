# Timeline::eq

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::eq(const Timeline& other) -> Series
```

## What It Does

Performs elementwise temporal equality and returns a boolean `Series`.

This page is also the canonical reference for the sibling comparison methods:

- `ne`
- `lt`
- `le`
- `gt`
- `ge`

All six methods share the same shape and broadcast contract. The only thing
that changes is the comparison relation.

## Parameters

**other**

The timeline to compare against.

This may be the same shape as `self`, or a scalar/broadcastable timeline when
the comparison is conceptually "compare every element to one timestamp".

Missing timestamps participate conservatively: a missing time does not compare
as less-than or greater-than any real timestamp.

## Returns

`Series`
    A boolean series describing the comparison result element by element.

## Examples

Example 1. Equality keeps only rows where the timestamps match exactly.

```cpp
auto out = base.eq(other);
std::cout << pluss::series::print(out) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, false, false]
```

Example 2. Less-than and greater-than comparisons answer true temporal ordering
questions.

```cpp
auto earlier = base.lt(other);
auto later_or_equal = base.ge(other);
std::cout << pluss::series::print(earlier) << "\n";
std::cout << pluss::series::print(later_or_equal) << "\n";
```

### Output

`base.lt(other)`:

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, true, false, true]
```

`base.ge(other)`:

```text
Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, true, false]
```

## Related Methods

- `ne(other)`
  True where timestamps differ.
- `lt(other)`
  True where `self < other`.
- `le(other)`
  True where `self <= other`.
- `gt(other)`
  True where `self > other`.
- `ge(other)`
  True where `self >= other`.

## Notes

- Comparison methods are usually the front door into mask-based temporal logic.
- Pair them with [`where`](where.md) when you want a temporal branch.
- Pair them with `DataFrame` or `Thread` filtering when the timeline lives in a
  table column.

## See Also

- [`Timeline::where`](where.md)
- [`Timeline::isnat`](isnat.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html">
    <span class="label">Previous</span>
    <span class="title">diff</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
