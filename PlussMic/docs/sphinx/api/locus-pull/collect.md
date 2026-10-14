<!-- pluss-hand-authored -->

# LocusPull::collect

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull::collect() const -> Catalog
```

## What It Does

Validates and executes the complete plan, returning one record per root
record. This is the only method that reads a stream, matches related
records, or reduces anything -- every other builder method only edits the
stored plan metadata.

Plain fields (`take()`) follow root-preserving links by composing the
matches directly, with no intermediate table built along the way; aggregates
reduce each root record's related records, over a window when the spec asks
for one. Output columns appear in the order their specs were added,
regardless of which builder method added them. `collect()` can be called
more than once on the same builder -- each call re-executes the current
plan, so a later `collect()` after more fields were added reflects them.

All plan-wide validation happens here, not when a field was added: an empty
plan, a duplicate output alias, an unknown table, and every
[`options()`](options.md) combination a pull doesn't support (see that page's
Example 4) are all caught at `collect()`.

## Parameters

None.

## Returns

A `Catalog` with one record per root record, columns in spec-addition order.

## Errors

Throws when the plan requests no fields at all, when two specs produce the
same output alias, when an unknown table is named, or when an
[`options()`](options.md) setting isn't valid for a pull.

## Examples

Example 1. Output column order follows spec-addition order, not builder-call
order within any one field's own expression.

```cpp
const auto ordered = data.pull("customers")
                         .take("regions.zone")
                         .take("id", "customer_id")
                         .sum("events.spend", "spent")
                         .collect();
print_keys(ordered);
```

### Output

```text
keys=[zone, customer_id, spent] rows=4
```

Example 2. Calling `collect()` more than once re-executes the current plan
-- a field added between calls shows up in the later one.

```cpp
auto reusable = data.pull("customers").take("id");
print_keys(reusable.collect());
print_keys(reusable.collect());
reusable.take("name");
print_keys(reusable.collect());
```

### Output

```text
keys=[id] rows=4
keys=[id] rows=4
keys=[id, name] rows=4
```

Example 3. Plan-wide failures: no fields at all, a duplicate output alias,
and an unknown table.

```cpp
print_error("empty", [&] { (void)data.pull("customers").collect(); });
print_error("alias", [&] {
  (void)data.pull("customers").take("id", "value").take("name", "value").collect();
});
print_error("unknown table", [&] { (void)data.pull("customers").take("ghost.value").collect(); });
```

### Output

```text
empty: Locus::pull: at least one field is required
alias: Locus::pull: output alias collision for 'value'
unknown table: Locus::pull: unknown catalog 'ghost'
```

## See Also

- [`take`](take.md), [`sum`](sum.md) and the other reducers: what builds the
  plan `collect()` executes.
- [`options`](options.md): the policy settings `collect()` validates.
