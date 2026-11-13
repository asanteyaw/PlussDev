<!-- pluss-hand-authored -->

# LocusPull::median

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull& LocusPull::median(std::string_view source, std::string_view alias = {})
```

## What It Does

The order-statistic median of the usable (non-null) related values of
`source` for each root record -- the middle value for an odd count, the
average of the two middle values for an even count. Requires a numeric
stream; see [`sum`](sum.md) for the shared null-for-empty and
`NaN`-exclusion rules. Omitting `alias` derives one from the source:
`"events.spend"` becomes `"events_spend_median"`.

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

Example 1. `median` against `mean`, on a customer with an even count (4
events: `2,4,6,8`) and one with an odd count (3 events: `5,8,9`).

```cpp
const auto reduced = data.pull("customers")
                         .mean("events.spend", "average")
                         .median("events.spend", "middle")
                         .collect();
print_numbers(reduced, "average");
print_numbers(reduced, "middle");
```

### Output

```text
average=[5, 7.33333333333, 4, null]
middle=[5, 8, 4, null]
```

Four values `2,4,6,8`: the middle two (`4`,`6`) average to `5`, matching the
mean here by coincidence of this data. Three values `5,8,9`: the middle
value is `8`, not the mean `7.333...` -- `median` and `mean` genuinely
differ once the count is odd.

Example 2. A text source is refused.

```cpp
print_error("text median", [&] {
  (void)data.pull("customers").median("events.tag", "value").collect();
});
```

### Output

```text
text median: Stream::reduce_groups: this reduction needs a numeric Series stream
```

## See Also

- [`mean`](mean.md): the arithmetic center, instead of the order-statistic
  one.
- [`sum`](sum.md): the shared null-for-empty and `NaN`-exclusion rules.
