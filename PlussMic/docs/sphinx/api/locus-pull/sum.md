<!-- pluss-hand-authored -->

# LocusPull::sum

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull& LocusPull::sum(std::string_view source, std::string_view alias = {})
```

## What It Does

Totals the usable (non-null) related values of `source` for each root
record. Unlike [`count`](count.md)/[`ndistinct`](ndistinct.md), `sum` and
every other reducer below it requires a numeric stream -- text and `Timeline`
sources throw at [`collect()`](collect.md). A root record with zero related
records sums to null, not zero; a root record with related records but every
one of them an invalid/missing numeric value sums to null too. Omitting
`alias` derives one from the source: `"events.spend"` becomes
`"events_spend_sum"`.

## Parameters

| Parameter | Meaning |
|---|---|
| `source` | A related, numeric field, written `"table.feature"`. |
| `alias` | The output column name. Empty derives one from `source`. |

## Returns

A reference to the same builder, so calls chain in place.

## Errors

Throws at [`collect()`](collect.md): an empty `source`, an unknown feature,
a non-numeric stream, or a `source` that is root-preserving rather than
reached through a one-to-many link (a plain root field is never a valid
reducer target).

## Examples

Example 1. `sum` alongside `count`, across customers with 4, 3, 1, and 0
events.

```cpp
const auto reduced = data.pull("customers")
                         .take("id")
                         .count("events.spend", "n")
                         .sum("events.spend", "total")
                         .collect();
print_numbers(reduced, "n");
print_numbers(reduced, "total");
```

### Output

```text
n=[4, 3, 1, 0]
total=[20, 22, 4, null]
```

The customer with zero events sums to `null`, not `0` -- a count of `0`
related records and a sum of `null` both say "nothing to total," in the
vocabulary each reducer uses.

Example 2. Missing (`NaN`) values among otherwise-valid numeric readings are
excluded, the same as a missing related record would be.

```cpp
const auto cleaned = data.pull("customers")
                         .count("events.reading", "usable")
                         .sum("events.reading", "total")
                         .collect();
print_numbers(cleaned, "usable");
print_numbers(cleaned, "total");
```

### Output

```text
usable=[3, 2, 0, 0]
total=[4, 10, null, null]
```

`events.reading` has two `NaN` entries among its eight values; `usable`
(from `count`) already excludes them, and `sum` excludes them too rather
than propagating `NaN` through the total.

Example 3. A text or `Timeline` source is refused -- `sum` needs a numeric
stream.

```cpp
print_error("text sum", [&] {
  (void)data.pull("customers").sum("events.tag", "value").collect();
});
```

### Output

```text
text sum: Stream::reduce_groups: this reduction needs a numeric Series stream
```

## See Also

- [`count`](count.md): a count of the same related records, with no numeric
  requirement.
- [`mean`](mean.md), [`median`](median.md), [`min`](min.md), [`max`](max.md),
  [`std`](std.md), [`var`](var.md): the other numeric reducers.
