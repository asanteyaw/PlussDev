<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 405866cfe223 -->

# Slot::unique

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
bool Slot::unique() const noexcept
```

## What It Does

Reports whether this `Slot` handle is the *sole owner* of its underlying
payload — a reference-count check, not a data check.

```{danger} Not about duplicate values — about shared ownership
`unique()` does **not** ask "does this slot's data contain duplicate
values." It asks "is this handle the only reference to its underlying
storage," the same sense as `std::shared_ptr::use_count() == 1`. Copying a
`Slot` (which is cheap — `Slot` shares its storage via an internal
intrusive pointer) makes `unique()` return `false` on *both* the original
and the copy, even though nothing about the data changed. If you're looking
for "does this slot have any repeated values," that's
[`distinct()`](distinct.md)/[`ndistinct()`](ndistinct.md), not this method.
```

## Parameters

`None`

## Returns

`bool`
    `true` if no other `Slot` handle currently shares this payload;
    `false` otherwise (including when the slot is undefined).

## Examples

Example 1. A freshly constructed slot is its own sole owner.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)});
std::cout << std::boolalpha << slot.unique();
```

### Output

```text
true
```

Example 2. Copying the slot means *neither* handle is unique anymore — see
the danger callout above.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)});
auto copy = slot;
std::cout << std::boolalpha << slot.unique() << " " << copy.unique();
```

### Output

```text
false false
```

## Notes

- See the danger callout above — this is about shared ownership, not
  duplicate data.
- `noexcept` — safe to call even on an undefined `Slot` (returns `false`).

## See Also

- [`Slot::defined`](defined.md): the other `noexcept` state check.
- [`Slot::distinct`](distinct.md) / [`Slot::ndistinct`](ndistinct.md): what
  to reach for when the goal is actually detecting duplicate values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="type_counts.html">
    <span class="label">Previous</span>
    <span class="title">type_counts</span>
  </a>
  <a class="next-link" href="values.html">
    <span class="label">Next</span>
    <span class="title">values</span>
  </a>
</div>
