<!-- pluss-hand-authored -->

# Timeline::defined

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
bool Timeline::defined() const noexcept
```

## What It Tells You

`defined()` answers whether the `Timeline` owns a valid temporal payload. It
is a state query, not an emptiness test and not a missing-value test.

A defined timeline may be scalar, empty, or contain `NaT`. Those are all valid
payload states. Use [`numel()`](numel.md) to ask how many positions exist and
[`isnat()`](isnat.md) to identify positions whose timestamp is missing.

Public factories return defined timelines or throw when construction fails, so
ordinary application code rarely needs to poll this method after every
factory call. It remains useful at API boundaries and in generic code that may
receive moved-from or conditionally initialized objects.

## Returns

`true` when the object has a valid backing payload; otherwise `false`. The
method does not allocate, inspect timestamp values, or perform timezone work.

## Examples

Example 1. Successful construction produces a defined object regardless of
rank.

```cpp
auto scalar = pluss::timeline::make("2024-02-29T12:30:00Z");
auto vector = pluss::timeline::make({
    "2024-02-29T12:30:00Z",
    "2024-03-01T00:00:00Z",
});

std::cout << std::boolalpha
          << "scalar defined=" << scalar.defined() << "\n"
          << "vector defined=" << vector.defined() << "\n";
```

### Output

```text
scalar defined=true
vector defined=true
```

Example 2. `NaT` is data inside a defined timeline; it does not make the
container undefined.

```cpp
auto times = pluss::timeline::make({
    "2024-02-29T12:30:00Z",
    "",
});

std::cout << std::boolalpha
          << "defined=" << times.defined()
          << ", positions=" << times.numel() << "\n";
pluss::series::print(std::cout, times.isnat());
std::cout << "\n";
```

### Output

```text
defined=true, positions=2
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[false, true]
```

## Gotchas And Failure Modes

- Do not use `defined()` to detect an empty timeline. A defined shape `[0]`
  has `numel() == 0`.
- Do not use it to detect `NaT`. Missing timestamps are valid stored values.
- A parse or shape error throws during construction; it does not normally
  return a public undefined sentinel for later inspection.

## Summary

`defined()` validates the object state. `numel()` validates cardinality, and
`isnat()` validates timestamp values; the three questions are deliberately
separate.

## See Also

- [`Timeline::numel`](numel.md)
- [`Timeline::isnat`](isnat.md)
- [`Timeline::isvalid`](isvalid.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../timeline_api_reference.html"><span class="label">Previous</span><span class="title">Timeline API Reference</span></a>
  <a class="next-link" href="sizes.html"><span class="label">Next</span><span class="title">sizes</span></a>
</div>
