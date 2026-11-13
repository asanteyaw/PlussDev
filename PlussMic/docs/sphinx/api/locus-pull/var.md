<!-- pluss-hand-authored -->

# LocusPull::var

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull& LocusPull::var(std::string_view source, std::string_view alias = {})
```

## What It Does

The sample variance of the usable (non-null) related values of `source` for
each root record -- [`std`](std.md) squared. Requires a numeric stream; see
[`sum`](sum.md) for the shared `NaN`-exclusion rule. Like `std`, `var` is
null for a root record with zero *or exactly one* related value, since
sample variance needs at least two observations. Omitting `alias` derives
one from the source: `"events.spend"` becomes `"events_spend_var"`.

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

Example 1. `var` against `std`, on the same data -- `var` is `std` squared.

```cpp
const auto reduced = data.pull("customers")
                         .std("events.spend", "sample_sd")
                         .var("events.spend", "sample_var")
                         .collect();
print_numbers(reduced, "sample_sd");
print_numbers(reduced, "sample_var");
```

### Output

```text
sample_sd=[2.58198889747, 2.08166599947, null, null]
sample_var=[6.66666666667, 4.33333333333, null, null]
```

`2.58198889747^2 = 6.66666666667`; `2.08166599947^2 = 4.33333333333` --
`var` and `std` agree exactly, as they must.

Example 2. A text source is refused.

```cpp
print_error("text var", [&] {
  (void)data.pull("customers").var("events.tag", "value").collect();
});
```

### Output

```text
text var: Stream::reduce_groups: this reduction needs a numeric Series stream
```

## See Also

- [`std`](std.md): the matching sample standard deviation.
- [`sum`](sum.md): the `NaN`-exclusion rule, and the reducers that only null
  out at zero related values.
