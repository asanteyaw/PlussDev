<!-- pluss-hand-authored -->

# Slot::values

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
const std::vector<Slot::Value>& Slot::values() const
```

## What It Does

Returns a read-only reference to the slot's underlying value vector —
`Slot::Value` is `std::variant<std::monostate, pluss::Series, pluss::Note,
pluss::Timeline, pluss::TimeIndex>`. This is the escape hatch for iterating
or inspecting a slot's raw contents directly, rather than through a
transform like [`filter()`](filter.md) or [`transform()`](transform.md).

## Parameters

`None`

## Returns

`const std::vector<Value>&`
    A reference to the slot's values, valid as long as the `Slot` itself is
    alive.

## Examples

Example 1. Iterate the values directly, extracting each `Series` scalar.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});

std::int64_t total = 0;
for (const auto& value : slot.values()) {
  total += std::get<pluss::Series>(value).to_vec<std::int64_t>()[0];
}
std::cout << "count=" << slot.values().size() << " total=" << total;
```

### Output

```text
count=3 total=6
```

Example 2. Check for a null value directly via `std::holds_alternative`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::bundle::Slot::Value{std::monostate{}}});

bool has_null = false;
for (const auto& value : slot.values()) {
  if (std::holds_alternative<std::monostate>(value)) has_null = true;
}
std::cout << std::boolalpha << has_null;
```

### Output

```text
true
```

## Notes

- Returns a reference, not a copy — cheap to call, but the reference is
  only valid as long as the source `Slot` is alive.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::at`](at.md): fetch a single value by index instead of the whole
  vector.
- [`Slot::filter`](filter.md) / [`Slot::transform`](transform.md): build a
  new `Slot` from a predicate or per-value function, instead of iterating
  manually.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unique.html">
    <span class="label">Previous</span>
    <span class="title">unique</span>
  </a>
</div>
