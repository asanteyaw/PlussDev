<!-- pluss-hand-authored -->

# LocusPull::options

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull::options(const PullOptions& options) -> LocusPull&
```

## What It Does

Replaces the plan's raggedness and root-key policy wholesale -- the most
recent call wins; `options()` does not merge with whatever was set before.

```cpp
struct PullOptions {
  RaggedPolicy ragged{RaggedPolicy::defaults(OpClass::Relational)};
  bool require_primary_key{true};
};
```

| Field | Meaning | Default |
|---|---|---|
| `ragged` | How root and related streams are read: domain, alignment, absent handling, materialize, loss. | Relational defaults (nothing dropped, null for a root record with no related record) |
| `require_primary_key` | Whether an aggregating pull needs the root table's declared primary key ([`Locus::meta`](../locus/catalogs.md)) to define one output record per root record. | `true` |

Not every `RaggedPolicy` setting is valid for a pull: `collect()` rejects
`RaggedAlignment::Label`, `RaggedDomain::Anchor`, any `RaggedAbsent` other
than `Null`, and any `RaggedNulls`/`min_support` other than the defaults --
see Example 4. Turning `require_primary_key` off lets an aggregating pull run
without a declared key, grouping by whatever the root record identity
happens to be, at the cost of the ambiguity a primary key exists to rule out.

## Parameters

| Parameter | Meaning |
|---|---|
| `options` | The complete replacement `PullOptions`. |

## Returns

A reference to the same builder, so calls chain in place.

## Errors

`options()` itself never throws -- an invalid combination (see Example 4)
throws at [`collect()`](collect.md).

## Examples

Example 1. `options()` returns the same builder and replaces, rather than
merges, the previous setting.

```cpp
auto option_plan = data.pull("customers").take("id");
PullOptions relaxed;
relaxed.require_primary_key = false;
auto* option_same = &option_plan.options(relaxed);
std::cout << "same_builder=" << std::boolalpha << (option_same == &option_plan) << " specs=" << option_plan.specs().size() << "\n";
PullOptions strict;
option_plan.options(strict);
print_numbers(option_plan.collect(), "id");
```

### Output

```text
same_builder=true specs=1
id=[1, 2, 3, 4]
```

The second `options(strict)` call replaces `relaxed` outright -- `specs()`
is untouched by either call, since options and specs are stored separately.

Example 2. `require_primary_key`: an aggregating pull on a root table with
no declared primary key is refused by default, and succeeds once the check
is turned off.

```cpp
print_error("strict", [&] { (void)no_meta.pull("customers").sum("events.spend", "spent").collect(); });
auto relaxed_plan = no_meta.pull("customers");
relaxed_plan.options(relaxed).sum("events.spend", "spent");
print_numbers(relaxed_plan.collect(), "spent");
```

### Output

```text
strict: Locus::pull: aggregating fields group related records under a root record, so root table 'customers' needs a declared primary key (Locus::meta), or turn off PullOptions::require_primary_key
spent=[20, 22, 4, null]
```

Example 3. `ragged.materialize = Pad` fills a shorter related stream with a
null instead of leaving it its own length.

```cpp
const auto preserved = ragged.pull("customers").take({"id", "name"}).collect();
std::cout << "preserve id=" << preserved.keys("id").size()
          << " name=" << preserved.keys("name").size() << " rows=" << preserved.maxrows() << "\n";
PullOptions pad;
pad.ragged.materialize = pluss::bundle::locus::RaggedMaterialize::Pad;
auto padded_plan = ragged.pull("customers");
padded_plan.options(pad).take({"id", "name"});
const auto padded = padded_plan.collect();
print_words(padded, "name");
```

### Output

```text
preserve id=4 name=2 rows=4
name=[Ada, Bola, null, null]
```

By default, a ragged root's own shorter streams stay their own length (`id`
has 4 records, `name` has 2) -- `Pad` fills `name` out to match `id`'s
length instead, with nulls for the missing tail.

Example 4. The combinations `collect()` rejects: `Label` alignment, a
non-`Null` `absent`, an `Anchor` domain, and a non-default `nulls`.

```cpp
PullOptions label;
label.ragged.alignment = pluss::bundle::locus::RaggedAlignment::Label;
auto label_plan = data.pull("customers");
label_plan.options(label).take("id");
print_error("label", [&] { (void)label_plan.collect(); });

PullOptions skip;
skip.ragged.absent = pluss::bundle::locus::RaggedAbsent::Skip;
auto skip_plan = data.pull("customers");
skip_plan.options(skip).take("id");
print_error("absent", [&] { (void)skip_plan.collect(); });

PullOptions anchor;
anchor.ragged.domain = pluss::bundle::locus::RaggedDomain::Anchor;
anchor.ragged.anchor_stream = "id";
auto anchor_plan = data.pull("customers");
anchor_plan.options(anchor).take("id");
print_error("anchor", [&] { (void)anchor_plan.collect(); });

PullOptions propagate;
propagate.ragged.nulls = pluss::bundle::locus::RaggedNulls::Propagate;
auto propagate_plan = data.pull("customers");
propagate_plan.options(propagate).take("id");
print_error("nulls", [&] { (void)propagate_plan.collect(); });
```

### Output

```text
label: Locus::pull: label alignment is not a pull mode
absent: Locus::pull: a root record with no related record is always an explicit null; absent must be Null
anchor: Locus::pull: an anchor-stream domain is not supported for pulls
nulls: Locus::pull: nulls and min_support do not apply to pulls
```

Each of these builds without complaint -- `options()` stores whatever it's
given -- and is rejected only once `collect()` checks the policy against
what a pull actually supports.

## See Also

- [`collect`](collect.md): where an invalid `options()` setting is actually
  caught.
- [Locus's own `with_policy`](../locus/state.md): the equivalent replacement
  for `Locus`-wide alignment, separate from a single pull's options.
