<!-- pluss-hand-authored -->

# LocusPull::mean

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull::mean(std::string_view source, std::string_view alias = {}) -> LocusPull&
```

## What It Does

The arithmetic mean of the usable (non-null) related values of `source` for
each root record. Requires a numeric stream, the same as [`sum`](sum.md) --
see that page for the shared null-for-empty and `NaN`-exclusion rules every
numeric reducer follows. Omitting `alias` derives one from the source:
`"events.spend"` becomes `"events_spend_mean"`.

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

Example 1. `mean` alongside `sum`, on the same customers (spend `2,4,6,8` /
`5,8,9` / `4` / none).

```cpp
const auto reduced = data.pull("customers")
                         .take("id")
                         .sum("events.spend", "total")
                         .mean("events.spend", "average")
                         .collect();
print_numbers(reduced, "total");
print_numbers(reduced, "average");
```

### Output

```text
total=[20, 22, 4, null]
average=[5, 7.33333333333, 4, null]
```

`20 / 4 = 5`; `22 / 3 = 7.333...`; one event's own value is its own mean;
zero events means no mean, same as no sum.

Example 2. A text source is refused.

```cpp
print_error("text mean", [&] {
  (void)data.pull("customers").mean("events.tag", "value").collect();
});
```

### Output

```text
text mean: Stream::reduce_groups: this reduction needs a numeric Series stream
```

## See Also

- [`sum`](sum.md): the null-for-empty and `NaN`-exclusion rules this page
  shares.
- [`median`](median.md): the order-statistic center, instead of the
  arithmetic one.
