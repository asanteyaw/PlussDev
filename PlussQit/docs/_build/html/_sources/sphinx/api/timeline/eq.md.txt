# Timeline::eq

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::eq(const Timeline& other) const
Series Timeline::operator==(const Timeline& other) const
```

## Exact Instant Equality

`eq()` compares paired UTC instants and returns one boolean per broadcasted
position. Timezone labels do not alter the answer: differently displayed local
times compare equal when they identify the same instant on the epoch axis.

The result is a boolean `Series`, not one `bool`:

```cpp
const auto times = pluss::timeline::make({
    "2024-01-01T00:00:00Z", "2024-01-01T02:00:00Z"});
const auto boundary = pluss::timeline::make("2024-01-01T02:00:00Z");

std::cout << pluss::series::print(times.eq(boundary)) << '\n';
std::cout << pluss::series::print(times == boundary) << '\n';
```

### Output

Both expressions produce:

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]
```

The scalar has shape `[]`; broadcasting pairs it with both elements and leaves
the result shape `[2]`. Higher-rank operands follow the same trailing-axis
rules. Shapes that cannot broadcast are rejected rather than truncated or
flattened.

## Equality Is About The Instant

```cpp
const auto utc = pluss::timeline::make("2024-07-01T18:00:00Z");
const auto local = utc.tz_convert("America/Edmonton");
std::cout << std::boolalpha << (utc == local).item<bool>() << '\n';
```

### Output

```text
true
```

`tz_convert()` changes local calendar presentation, not the represented
instant. Compare `tz()` separately if timezone metadata is part of an
application invariant.

## `NaT` And Aggregate Decisions

An absent instant is not equal to anything, including another `NaT`:

```cpp
const auto values = pluss::timeline::make({"", "2024-01-01T00:00:00Z"});
std::cout << pluss::series::print(values == values) << '\n';
```

### Output

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]
```

Use `(a == b).all().item<bool>()` for one whole-array decision. If absent
positions require special equivalence, inspect `isnat()` and encode that policy
in the mask before reducing.

## Failure Modes

- Both operands must be defined Timeline values.
- Their shapes must be broadcast-compatible.
- Equality does not parse text; construct a Timeline first.

## See Also

- [`Timeline operators`](operators.md)
- [`Timeline::ne`](ne.md)
- [`Timeline::isnat`](isnat.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html"><span class="label">Previous</span><span class="title">diff</span></a>
  <a class="next-link" href="fill_like.html"><span class="label">Next</span><span class="title">fill_like</span></a>
</div>
