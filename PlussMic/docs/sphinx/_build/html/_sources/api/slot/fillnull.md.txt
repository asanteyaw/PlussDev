<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1a666b51e359 -->

# Slot::fillnull

[Back to Slot API Reference](../../slot_api_reference)

## Signatures

```cpp
Slot Slot::fillnull(double value) const
Slot Slot::fillnull(std::string_view value) const
Slot Slot::fillnull(const char* value) const
Slot Slot::fillnull(const pluss::Timeline& value) const
Slot Slot::fillnull(const pluss::TimeIndex& value) const
```

## What It Does

Returns a new slot with every null (`std::monostate`) value replaced by
`value` — one overload per fill-value kind: `double` for numeric slots,
`std::string_view`/`const char*` for text slots, `Timeline`/`TimeIndex`
for datetime slots.

```{important} Every non-null value must already match the fill kind
`fillnull()` isn't purely additive — it requires every existing, non-null
value in the slot to already be compatible with the chosen overload's kind,
even the ones that have nothing to do with the fill. Calling
`fillnull(double)` on a slot that holds even one `Note` value throws,
regardless of whether that `Note` value is itself null. Pick the overload
that matches the slot's actual [`kind()`](kind.md).
```

## Parameters

**value** (`double` / `std::string_view` / `const char*` / `const pluss::Timeline&` / `const pluss::TimeIndex&`)
    The replacement for null values, in the kind matching the overload
    selected.

## Returns

`Slot`
    A new slot, same length as the source, with every null replaced by
    `value`.

## Examples

Example 1. Fill a numeric slot's null with `99.0`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::bundle::Slot::Value{std::monostate{}}, pluss::make_series(3)});
auto out = slot.fillnull(99.0);
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(2)).to_vec<std::int64_t>()[0];
```

### Output

```text
1 99 3
```

Example 2. Fill a text slot's null with a placeholder string (the `const
char*` overload).

```cpp
auto notes = pluss::bundle::Slot({pluss::make_note("a"), pluss::bundle::Slot::Value{std::monostate{}}});
auto out = notes.fillnull("N/A");
std::cout << std::get<pluss::Note>(out.at(1)).at(0);
```

### Output

```text
N/A
```

Example 3. Calling `fillnull(double)` on a text slot throws — see the
important callout above.

```cpp
auto notes = pluss::bundle::Slot({pluss::make_note("a"), pluss::bundle::Slot::Value{std::monostate{}}});
try {
  auto out = notes.fillnull(1.0);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slot::fillnull(double): non-null Slot values must match fill payload type
```

## Notes

- See the important callout above — pick the overload matching the slot's
  actual kind, not just the kind you intend to fill with.
- Filling an `int64` `Series` slot's null with a `double` only promotes
  the *filled* value itself to `Series[float64]` — the slot's original
  `int64` values are untouched, since each value in a `Slot` carries its
  own independent dtype rather than one shared column dtype.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::isnull`](isnull.md): identify which values are null before
  deciding how to fill them.
- [`Slot::dropnull`](dropnull.md): remove nulls instead of filling them,
  with no kind-matching requirement on the rest of the slot.
- [`Registry::fillnull`](../registry/fillnull.md): the field-scoped
  equivalent, with the same important callout applying per field.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html">
    <span class="label">Previous</span>
    <span class="title">empty</span>
  </a>
  <a class="next-link" href="filter.html">
    <span class="label">Next</span>
    <span class="title">filter</span>
  </a>
</div>
