<!-- pluss-hand-authored -->

# LocusPull::max

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull::max(std::string_view source, std::string_view alias = {}) -> LocusPull&
```

## What It Does

The highest usable (non-null) related value of `source` for each root
record. Requires a numeric stream; see [`sum`](sum.md) for the shared
null-for-empty and `NaN`-exclusion rules. Omitting `alias` derives one from
the source: `"events.spend"` becomes `"events_spend_max"`.

## Parameters

| Parameter | Meaning |
|---|---|
| `source` | A related, numeric field, written `"table.feature"`. |
| `alias` | The output column name. Empty derives one from `source`. |

## Returns

A reference to the same builder, so calls chain in place.

## Errors

Throws at [`collect()`](collect.md): an empty `source`, an unknown feature,
a non-numeric stream, or a root-preserving `source`.

## Examples

Example 1. `max` against `min` -- see [`min`](min.md)'s own Example 1 for
the paired output on the same data (`low=[2, 5, 4, null]`,
`high=[8, 9, 4, null]`).

```cpp
const auto reduced = data.pull("customers")
                         .min("events.spend", "low")
                         .max("events.spend", "high")
                         .collect();
print_numbers(reduced, "high");
```

### Output

```text
high=[8, 9, 4, null]
```

Example 2. A text source is refused.

```cpp
print_error("text max", [&] {
  (void)data.pull("customers").max("events.tag", "value").collect();
});
```

### Output

```text
text max: Stream::reduce_groups: this reduction needs a numeric Series stream
```

## See Also

- [`min`](min.md): the matching lower extreme.
- [`sum`](sum.md): the shared null-for-empty and `NaN`-exclusion rules.
