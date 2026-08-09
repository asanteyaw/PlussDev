<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9cd78f2e6f99 -->

# Slot::filter

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot Slot::filter(const std::function<bool(const Value&)>& predicate) const
```

## What It Does

Returns a new slot keeping only the values for which `predicate` returns
`true`.

## Parameters

**predicate** (`const std::function<bool(const Slot::Value&)>&`)
    Called once per value; return `true` to keep it, `false` to drop it.
    Cannot be an empty `std::function`.

## Returns

`Slot`
    A new slot with only the values that satisfied `predicate`.

## Examples

Example 1. Keep only the `Series` values out of a mixed-kind slot.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("x"), pluss::make_series(3)});
auto out = slot.filter([](const pluss::bundle::Slot::Value& value) {
  return std::holds_alternative<pluss::Series>(value);
});
std::cout << "size=" << out.size() << " vals="
          << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << ","
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
size=2 vals=1,3
```

Example 2. An empty predicate throws.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1)});
try {
  std::function<bool(const pluss::bundle::Slot::Value&)> empty_pred;
  slot.filter(empty_pred);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slot::filter: predicate is empty
```

## Notes

- The relative order of kept values is preserved.
- An empty `std::function` throws `"Slot::filter: predicate is empty"`
  rather than being treated as "keep everything" or "keep nothing."
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::transform`](transform.md): change each value in place instead
  of dropping some of them.
- [`Slot::dropnull`](dropnull.md): the common case of filtering out null
  values, already built in.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
