<!-- pluss-hand-authored -->

# LocusPull::std

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull& LocusPull::std(std::string_view source, std::string_view alias = {})
```

## What It Does

The sample standard deviation of the usable (non-null) related values of
`source` for each root record. Requires a numeric stream; see
[`sum`](sum.md) for the shared `NaN`-exclusion rule. Unlike `sum`/`mean`/
`median`/`min`/`max`, which are only null when a root record has *zero*
related values, `std` (and [`var`](var.md)) is also null for exactly *one*
related value -- sample variance is undefined for a single observation, not
just an empty one. Omitting `alias` derives one from the source:
`"events.spend"` becomes `"events_spend_std"`.

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

Example 1. `std` across customers with 4, 3, 1, and 0 events -- null starts
at the *one*-event customer, not only the zero-event one.

```cpp
const auto reduced = data.pull("customers")
                         .count("events.spend", "n")
                         .std("events.spend", "sample_sd")
                         .collect();
print_numbers(reduced, "n");
print_numbers(reduced, "sample_sd");
```

### Output

```text
n=[4, 3, 1, 0]
sample_sd=[2.58198889747, 2.08166599947, null, null]
```

Compare [`sum`](sum.md)'s Example 1 on the same data: `total=[20, 22, 4,
null]` is only null at `n=0`. `sample_sd` is null at both `n=1` and `n=0`.

Example 2. A text source is refused.

```cpp
print_error("text std", [&] {
  (void)data.pull("customers").std("events.tag", "value").collect();
});
```

### Output

```text
text std: Stream::reduce_groups: this reduction needs a numeric Series stream
```

## See Also

- [`var`](var.md): the matching sample variance (`std` squared).
- [`sum`](sum.md): the `NaN`-exclusion rule, and the reducers that only null
  out at zero related values.
