<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 879f78c5f493 -->

# Slot::empty

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
bool Slot::empty() const
```

## What It Does

Reports whether the slot holds zero values. Shorthand for
`size() == 0`.

## Parameters

`None`

## Returns

`bool`
    `true` if the slot has no values; `false` otherwise.

## Examples

Example 1. A slot with values is not empty.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1)});
std::cout << std::boolalpha << slot.empty();
```

### Output

```text
false
```

Example 2. A zero-value (but defined) slot is empty.

```cpp
auto slot = pluss::scribble::Slot(std::vector<pluss::scribble::Slot::Value>{});
std::cout << std::boolalpha << slot.empty();
```

### Output

```text
true
```

## Notes

- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"` (via `size()`, which `empty()` calls).
- Different from [`defined()`](defined.md): a slot can be defined and
  empty at the same time (zero values, but still a valid handle).

## See Also

- [`Slot::size`](size.md): the count `empty()` checks against zero.
- [`Slot::defined`](defined.md): checks handle validity, not value count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
