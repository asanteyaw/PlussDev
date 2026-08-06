<!-- pluss-hand-authored -->

# Slot::transform

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::transform(const std::function<Value(const Value&)>& fn) const
```

## What It Does

Returns a new slot with `fn` applied to every value. The most general,
escape-hatch way to build a per-value transform when no built-in method
(like [`nt()`](nt.md) or [`tl()`](tl.md)) covers the operation.

## Parameters

**fn** (`const std::function<Slot::Value(const Slot::Value&)>&`)
    Called once per value; its return value replaces the original. Cannot
    be an empty `std::function`.

## Returns

`Slot`
    A new slot, same length as the source, with each value replaced by
    `fn`'s result.

## Examples

Example 1. Multiply every `Series` scalar by 10.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
auto out = slot.transform([](const pluss::bundle::Slot::Value& value) -> pluss::bundle::Slot::Value {
  auto n = std::get<pluss::Series>(value).to_vec<std::int64_t>()[0];
  return pluss::make_series(n * 10);
});
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(2)).to_vec<std::int64_t>()[0];
```

### Output

```text
10,20,30
```

## Notes

- Always produces a slot the same length as the source — `fn` replaces
  values, it doesn't drop or add them (use [`filter()`](filter.md) to drop
  values).
- `fn` may return a different value kind than its input — the resulting
  slot's [`kind()`](kind.md) reflects whatever mix of kinds `fn` actually
  produced.
- An empty `std::function` throws `"Slot::transform: fn is empty"`.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::filter`](filter.md): keep or drop values rather than transform
  them.
- [`Slot::nt`](nt.md) / [`Slot::tl`](tl.md): purpose-built accessors for
  text and datetime transforms — reach for `transform()` only when those
  don't cover what's needed.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="type_counts.html">
    <span class="label">Next</span>
    <span class="title">type_counts</span>
  </a>
</div>
