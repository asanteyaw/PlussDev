<!-- pluss-hand-authored -->

# LocusPull::add

[Back to LocusPull API Reference](../../locus_pull_api_reference)

## Signature

```cpp
LocusPull& LocusPull::add(const PullSpec& spec)
LocusPull& LocusPull::add(std::initializer_list<PullSpec> specs)
```

## What It Does

Appends one or more fully-formed `PullSpec` values to the plan directly. Every
other builder method (`take`, `count`, `sum`, and the rest) is sugar that
constructs a `PullSpec` through the matching `nexus::` factory function
(`nexus::take`, `nexus::sum`, ...) and calls `add()` with it -- `add()` is the
one place the builder actually mutates its stored spec list.

`PullSpec` is a plain, independently-constructible value:

### `PullSpec` fields

| Field | Type | Default |
|---|---|---|
| `source` | `std::string` | empty |
| `op` | `PullOp` | `PullOp::Take` |
| `alias` | `std::optional<std::string>` | empty |
| `window` | `std::optional<PullWindow>` | empty |
| `named` | `Method` | `PullSpec named(std::string_view value) const` |
| `windowed` | `Method` | `PullSpec windowed(std::int64_t value, std::string_view unit) const` |
| `root_windowed` | `Method` | `PullSpec root_windowed(std::int64_t value, std::string_view unit) const` |

| Field | Meaning | Default |
|---|---|---|
| `source` | The field expression (`"id"`, `"regions.zone"`). | empty |
| `op` | `Take`, `Count`, `Ndistinct`, `Sum`, `Mean`, `Median`, `Min`, `Max`, `Std`, or `Var`. | `Take` |
| `alias` | The output column name. Unset means "use `source`'s own name." | unset |
| `window` | A trailing-window scope, for reducers only. | unset |

`named()`, `windowed()`, and `root_windowed()` each return a modified copy of
the spec, so they chain off a `nexus::` factory call: `nexus::sum("events.spend")
.named("recent").windowed(7, "d")`. `windowed(n, unit)` scopes the reduction to
the `n`-`unit` window ending at each related record's own latest timestamp;
`root_windowed` anchors that window to the root record's own time field
instead.

`add()` appends without validating the spec -- an empty `source`, a duplicate
alias, or an unreachable field are all accepted here and only rejected when
[`collect()`](collect.md) actually runs the plan. Copying a `PullSpec` into
`add()` takes an independent value: mutating the original afterward does not
change what was stored.

## Parameters

| Parameter | Meaning |
|---|---|
| `spec` | One complete `PullSpec`. |
| `specs` | Several `PullSpec` values, added in order. |

## Returns

A reference to the same builder, so calls chain in place.

## Errors

`add()` itself never throws. A spec it stored that turns out malformed (empty
source, duplicate output alias, and the rest) throws when
[`collect()`](collect.md) runs.

## Examples

Example 1. A single `add()` and a batch `add()`, both via the `nexus::take`/
`nexus::sum` factories. `add()` returns the same builder (`same_builder`
confirms it's not a copy).

```cpp
auto added = data.pull("customers");
auto* same = &added.add(nx::take("id", "customer_id"));
added.add({nx::take("name", "customer_name"), nx::sum("events.spend", "spent")});
std::cout << "same_builder=" << std::boolalpha << (same == &added) << "\n";
print_specs(added.specs());
print_keys(added.collect());
```

### Output

```text
same_builder=true
specs=3
source=id op=take alias=customer_id
source=name op=take alias=customer_name
source=events.spend op=sum alias=spent
keys=[customer_id, customer_name, spent] rows=4
```

Example 2. `named()`/`windowed()`/`root_windowed()` chained onto a factory
spec before `add()`.

```cpp
auto windowed = data.pull("customers");
windowed.add(nx::sum("events.spend").named("recent").windowed(7, "d"));
windowed.add(nx::sum("events.spend").named("before_signup").root_windowed(7, "d"));
print_specs(windowed.specs());
const auto window_result = windowed.collect();
print_numbers(window_result, "recent");
print_numbers(window_result, "before_signup");
```

### Output

```text
specs=2
source=events.spend op=sum alias=recent window=7d anchor=latest
source=events.spend op=sum alias=before_signup window=7d anchor=root
recent=[14, 17, 4, null]
before_signup=[4, 13, 4, null]
```

`windowed` (`anchor=latest`) sums each customer's spend over the 7 days ending
at that *event's own* timestamp; `root_windowed` (`anchor=root`) anchors the
same 7-day window to the *customer's* signup date instead -- different
customers, different values.

Example 3. `add()` stores a malformed or duplicate spec without complaint;
the error surfaces at `collect()`.

```cpp
auto malformed = data.pull("customers");
malformed.add(PullSpec{});
std::cout << "malformed plan specs=" << malformed.specs().size() << "\n";
print_error("malformed collect", [&] { (void)malformed.collect(); });

auto duplicate = data.pull("customers");
duplicate.add({nx::take("id"), nx::take("id")});
print_error("duplicate", [&] { (void)duplicate.collect(); });
```

### Output

```text
malformed plan specs=1
malformed collect: Locus::pull: pull spec source cannot be empty
duplicate: Locus::pull: duplicate requested field 'customers.id'
```

A default-constructed `PullSpec{}` (empty `source`) is accepted by `add()` --
`specs()` reports `1` -- and only rejected once `collect()` tries to resolve
it.

Example 4. `add(spec)` copies the value; mutating the original afterward does
not change what was stored.

```cpp
auto source_spec = nx::take("id", "customer_id");
auto copied = data.pull("customers");
copied.add(source_spec);
source_spec.source = "name";
source_spec.alias = "changed";
print_specs(copied.specs());
```

### Output

```text
specs=1
source=id op=take alias=customer_id
```

`copied`'s stored spec still reads `id`/`customer_id` -- the mutation to
`source_spec` afterward reached only the local variable.

## See Also

- [`take`](take.md), [`sum`](sum.md), and the other reducers: the convenience
  spellings that build a `PullSpec` and call `add()` for you.
- [`specs`](specs.md): reading the stored spec list back without executing it.
- [`collect`](collect.md): where a stored spec is actually validated.
