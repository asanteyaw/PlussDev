<!-- pluss-hand-authored -->

# LocusPull::count

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull::count(std::string_view source, std::string_view alias = {}) -> LocusPull&
```

## What It Does

Counts the usable (non-null) related values of `source` for each root
record -- the fluent spelling of `add(nexus::count(source, alias))`. Unlike
the other reducers, `count` (and [`ndistinct`](ndistinct.md)) works on *any*
stream kind -- numeric, text, or `Timeline` -- since counting occurrences
doesn't need the values themselves to support arithmetic. Omitting `alias`
derives one from the source: `"events.event_id"` becomes
`"events_event_id_count"`.

Like every other reducer, `source` must be a field reached through a
one-to-many (or many-to-many) relationship from root -- counting a
root-preserving field is refused the same way [`take`](take.md) refuses a
*plain* read across a one-to-many link, just for the opposite reason: a
root-preserving field already has exactly one value per root record, so
counting it is meaningless.

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

Example 1. `count` alongside the other numeric reducers, on a customer with
4 events, one with 3, one with 1, and one with none.

```cpp
const auto reduced = data.pull("customers")
                         .take("id")
                         .count("events.spend", "n")
                         .sum("events.spend", "total")
                         .collect();
print_numbers(reduced, "n");
```

### Output

```text
n=[4, 3, 1, 0]
```

Example 2. `count` works on text and `Timeline` streams too, not just
numeric ones -- unlike `sum`/`mean`/the other numeric reducers.

```cpp
const auto counted = data.pull("customers")
                         .count("events.tag", "tag_count")
                         .count("events.at", "time_count")
                         .collect();
print_numbers(counted, "tag_count");
print_numbers(counted, "time_count");
```

### Output

```text
tag_count=[4, 3, 1, 0]
time_count=[4, 3, 1, 0]
```

`events.tag` (text) and `events.at` (`Timeline`) both count cleanly -- the
same four values `events.spend`'s own `count` reported in Example 1, since
every event has a tag and a timestamp.

Example 3. The default alias, when none is given.

```cpp
const auto defaults = data.pull("customers").count("events.event_id").collect();
print_keys(defaults);
```

### Output

```text
keys=[events_event_id_count] rows=4
```

## See Also

- [`ndistinct`](ndistinct.md): the other stream-kind-agnostic reducer.
- [`sum`](sum.md) and the rest: the numeric-only reducers.
- [`agg`](agg.md): scoping several reducers to one table at once.
