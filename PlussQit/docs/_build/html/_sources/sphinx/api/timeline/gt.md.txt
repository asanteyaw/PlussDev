# Timeline::gt

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::gt(const Timeline& other) const
Series Timeline::operator>(const Timeline& other) const
```

## Instants After A Boundary

`gt()` reports where the left instant occurs strictly after the right instant.
It is the strict lower-bound relation.

```cpp
const auto times = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
const auto start = pluss::timeline::make("2024-01-01T00:00:00Z");
std::cout << pluss::series::print(times > start) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]
```

Equality at the first position is excluded. Use `ge()` to include it.

## Pairwise And Broadcasted Questions

A scalar boundary is only the simplest case. Two equally shaped Timelines are
compared position by position, and lower-rank operands broadcast across
compatible trailing dimensions. The operation never searches for “the next”
instant; it only compares the pairs established by broadcasting.

`NaT` is unordered, so either operand being absent produces `false`. Use
`isnat()` alongside `gt()` if absence must be represented as a separate state.

Non-broadcastable shapes throw before a mask is returned.

## See Also

- [`Timeline::ge`](ge.md)
- [`Timeline::lt`](lt.md)
- [`Timeline::max`](max.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html"><span class="label">Previous</span><span class="title">le</span></a>
  <a class="next-link" href="ge.html"><span class="label">Next</span><span class="title">ge</span></a>
</div>
