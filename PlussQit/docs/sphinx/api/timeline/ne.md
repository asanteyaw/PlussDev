# Timeline::ne

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::ne(const Timeline& other) const
Series Timeline::operator!=(const Timeline& other) const
```

## Detecting Changed Or Absent Positions

`ne()` returns `true` wherever paired instants differ. It is elementwise and
broadcasting-aware; `operator!=` has the same contract.

```cpp
const auto times = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
const auto boundary = pluss::timeline::make("2024-01-01T02:00:00Z");
std::cout << pluss::series::print(times != boundary) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, false]
```

This asks which positions are not the boundary instant. To ask whether the two
Timeline values differ anywhere, reduce explicitly with `any()`.

## `NaT` Is Unequal

`NaT != value` is `true`, including when `value` is also `NaT`:

```cpp
const auto values = pluss::timeline::make({"", "2024-01-01T00:00:00Z"});
std::cout << pluss::series::print(values != values) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, false]
```

If two absent positions should count as equivalent in a particular algorithm,
state that policy by combining `eq()` with both `isnat()` masks.

## Failure Modes

- Non-broadcastable shapes are rejected.
- There is no string overload; construct a Timeline before comparing.
- Scalar inputs still return a scalar `Series`; use `item<bool>()` to extract
  its native value.

## See Also

- [`Timeline::eq`](eq.md)
- [`Timeline::isnat`](isnat.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html"><span class="label">Previous</span><span class="title">eq</span></a>
  <a class="next-link" href="lt.html"><span class="label">Next</span><span class="title">lt</span></a>
</div>
