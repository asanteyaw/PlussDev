<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ccee6a2baab4 -->

# Slot::dropnull

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::dropnull() const
```

## What It Does

Returns a new slot with every null (`std::monostate`) value removed,
shortening the slot.

## Parameters

`None`

## Returns

`Slot`
    A new slot, holding only the non-null values, in their original order.

## Examples

Example 1. Drop the null in the middle of a slot.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::scribble::Slot::Value{std::monostate{}}, pluss::make_series(3)});
auto out = slot.dropnull();
std::cout << "size=" << out.size() << " vals="
          << std::get<pluss::Series>(out.at(0)).host<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(1)).host<std::int64_t>()[0];
```

### Output

```text
size=2 vals=1,3
```

## Notes

- Unlike [`fillnull()`](fillnull.md), `dropnull()` has no kind-matching
  requirement on the rest of the slot — it works on any slot regardless of
  what non-null values it holds.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::isnull`](isnull.md): identify nulls without removing them.
- [`Slot::fillnull`](fillnull.md): replace nulls instead of dropping them,
  keeping the slot's original length.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="distinct.html">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="empty.html">
    <span class="label">Next</span>
    <span class="title">empty</span>
  </a>
</div>
