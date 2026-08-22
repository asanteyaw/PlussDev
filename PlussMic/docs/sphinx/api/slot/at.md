<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9d7d0423d912 -->

# Slot::at

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
const Slot::Value& Slot::at(std::int64_t i) const
```

## What It Does

Fetches the value at position `i`, bounds-checked.

## Parameters

**i** (`std::int64_t`)
    The zero-based index to fetch.

## Returns

`const Value&`
    A reference to the value at index `i`.

## Examples

Example 1. Fetch and unwrap a `Series` value.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
std::cout << std::get<pluss::Series>(slot.at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
1
```

Example 2. An out-of-bounds index throws.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1)});
try {
  auto v = slot.at(5);
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

- Bounds-checked: negative indices and indices at or beyond `size()` both
  throw `"Slot::at: index out of bounds"` — there's no Python-style
  negative-index wraparound.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::values`](values.md): fetch every value at once rather than one
  at a time.
- [`Slot::size`](size.md): the upper bound `i` must stay below.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="astype.html">
    <span class="label">Previous</span>
    <span class="title">astype</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
