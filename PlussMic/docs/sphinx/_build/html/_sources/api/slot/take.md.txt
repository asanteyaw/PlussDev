<!-- pluss-hand-authored -->

# Slot::take

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::take(std::span<const std::int64_t> indices) const
```

## What It Does

Returns a new slot holding the values at each position in `indices`, in
the order given — an arbitrary (not necessarily contiguous or sorted)
positional selection, with repeats allowed.

## Parameters

**indices** (`std::span<const std::int64_t>`)
    The positions to select, in the order the result should have them.
    Repeats are allowed.

## Returns

`Slot`
    A new slot with one value per entry in `indices`.

## Examples

Example 1. Select positions out of order, with a repeat.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
std::vector<std::int64_t> picks{2, 0, 0};
auto out = slot.take(picks);
std::cout << "size=" << out.size() << " vals="
          << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(2)).to_vec<std::int64_t>()[0];
```

### Output

```text
size=3 vals=3,1,1
```

Example 2. An out-of-range index throws — unlike [`slice()`](slice.md),
`take()` doesn't clamp.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1)});
try {
  std::vector<std::int64_t> bad{10};
  auto out = slot.take(bad);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slot::at: index out of bounds
```

## Notes

- No clamping and no negative-index support — every index is checked via
  [`at()`](at.md), so an out-of-range index throws
  `"Slot::at: index out of bounds"` rather than being silently dropped or
  clipped.
- Order and repeats in `indices` are both preserved in the result.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::slice`](slice.md): a contiguous-range selection that clamps
  instead of throwing.
- [`Slot::at`](at.md): the single-value fetch `take()` uses internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="tl.html">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>
