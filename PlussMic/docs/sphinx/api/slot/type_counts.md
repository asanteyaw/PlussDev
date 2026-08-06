<!-- pluss-hand-authored -->

# Slot::type_counts

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
std::unordered_map<std::string, std::int64_t> Slot::type_counts() const
```

## What It Does

Returns a tally of how many values fall under each value-type name
(`"Series"`, `"Note"`, `"Timeline"`, `"TimeIndex"`, or `"null"` for
`std::monostate`).

## Parameters

`None`

## Returns

`std::unordered_map<std::string, std::int64_t>`
    A map from type name to count.

## Examples

Example 1. Tally a slot with two `Series` values and one `Note`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_note("ok"), pluss::make_series(3)});
auto counts = slot.type_counts();
std::cout << "Series=" << counts.at("Series") << " Note=" << counts.at("Note");
```

### Output

```text
Series=2 Note=1
```

## Notes

- Returns a `std::unordered_map` — iteration order is unspecified; look up
  specific keys rather than relying on insertion or alphabetical order.
- A type with zero occurrences simply doesn't appear as a key — use
  `.count(key)` or `.find(key)` rather than `operator[]` if a missing key
  should read as `0` instead of default-constructing an entry.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::kind`](kind.md) / [`Slot::ptype`](ptype.md): a single overall
  classification, rather than a per-type breakdown.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transform.html">
    <span class="label">Previous</span>
    <span class="title">transform</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
