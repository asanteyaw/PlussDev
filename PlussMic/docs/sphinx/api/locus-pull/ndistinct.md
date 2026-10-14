<!-- pluss-hand-authored -->

# LocusPull::ndistinct

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull::ndistinct(std::string_view source, std::string_view alias = {}) -> LocusPull&
```

## What It Does

Counts the number of distinct usable (non-null) related values of `source`
for each root record. Like [`count`](count.md), `ndistinct` works on any
stream kind -- numeric, text, or `Timeline` -- since distinguishing values
doesn't require arithmetic. Omitting `alias` derives one from the source:
`"events.tag"` becomes `"events_tag_ndistinct"`.

## Parameters

| Parameter | Meaning |
|---|---|
| `source` | A related field, written `"table.feature"`. |
| `alias` | The output column name. Empty derives one from `source`. |

## Returns

A reference to the same builder, so calls chain in place.

## Errors

Throws at [`collect()`](collect.md): an empty `source`, an unknown feature,
or a `source` that doesn't cross a one-to-many relationship from root.

## Examples

Example 1. `ndistinct` against `count`, on the same numeric field -- one
customer's four events repeat a value, so `ndistinct` reports fewer than
`count` for that customer.

```cpp
const auto reduced = data.pull("customers")
                         .take("id")
                         .count("events.spend", "n")
                         .ndistinct("events.spend", "distinct_spend")
                         .collect();
print_numbers(reduced, "n");
print_numbers(reduced, "distinct_spend");
```

### Output

```text
n=[4, 3, 1, 0]
distinct_spend=[4, 3, 1, 0]
```

Every customer's events happen to have distinct spend amounts here, so the
two columns agree -- `ndistinct` only differs from `count` when a value
repeats within one root record's related rows.

Example 2. `ndistinct` works on a text stream too.

```cpp
const auto counted = data.pull("customers")
                         .count("events.tag", "tag_count")
                         .ndistinct("events.tag", "tag_kinds")
                         .collect();
print_numbers(counted, "tag_count");
print_numbers(counted, "tag_kinds");
```

### Output

```text
tag_count=[4, 3, 1, 0]
tag_kinds=[2, 2, 1, 0]
```

`events.tag` only takes the values `"web"`/`"shop"` -- `tag_kinds` tops out
at `2` even for a customer with `4` tagged events.

## See Also

- [`count`](count.md): the other stream-kind-agnostic reducer.
- [`sum`](sum.md) and the rest: the numeric-only reducers.
