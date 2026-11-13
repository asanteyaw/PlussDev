<!-- pluss-hand-authored -->

# LocusPull::min

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull& LocusPull::min(std::string_view source, std::string_view alias = {})
```

## What It Does

The lowest usable (non-null) related value of `source` for each root record.
Requires a numeric stream; see [`sum`](sum.md) for the shared null-for-empty
and `NaN`-exclusion rules. Omitting `alias` derives one from the source:
`"events.spend"` becomes `"events_spend_min"`.

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

Example 1. `min` against `max`, on the same spend values (`2,4,6,8` /
`5,8,9` / `4` / none).

```cpp
const auto reduced = data.pull("customers")
                         .min("events.spend", "low")
                         .max("events.spend", "high")
                         .collect();
print_numbers(reduced, "low");
print_numbers(reduced, "high");
```

### Output

```text
low=[2, 5, 4, null]
high=[8, 9, 4, null]
```

A customer with one event has the same `min` and `max` -- that event's own
value. Zero events has neither.

Example 2. A text source is refused.

```cpp
print_error("text min", [&] {
  (void)data.pull("customers").min("events.tag", "value").collect();
});
```

### Output

```text
text min: Stream::reduce_groups: this reduction needs a numeric Series stream
```

## See Also

- [`max`](max.md): the matching upper extreme.
- [`sum`](sum.md): the shared null-for-empty and `NaN`-exclusion rules.
