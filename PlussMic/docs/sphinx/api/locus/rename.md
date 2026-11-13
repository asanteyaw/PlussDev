<!-- pluss-hand-authored -->

# Locus::rename, Locus::rename_

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus Locus::rename(std::string_view from, std::string_view to) const
Locus& Locus::rename_(std::string_view from, std::string_view to)
```

## What It Does

Re-keys a catalog from `from` to `to`. Every declared link, table metadata
entry, and saved join plan that named `from` is updated to name `to`
instead -- a join that worked before a rename keeps working after it, with
no need to redeclare anything. `rename_` is the in-place form.

## Parameters

| Parameter | Meaning |
|---|---|
| `from` | The catalog's current name. |
| `to` | Its new name. Must differ from `from` and must not already be in use. |

## Returns

`rename` returns a new `Locus`. `rename_` returns a reference to the mutated
receiver.

## Errors

Throws when `to` is already in use, or when `from` and `to` are the same
name.

## Examples

Example 1. Renaming `customers` to `people` -- the link that pointed at
`customers` now points at `people`.

```cpp
const auto linked = make_linked_shop();
const auto people = linked.rename("customers", "people");
std::cout << "people has(people)=" << people.has("people") << " has(customers)=" << people.has("customers") << "\n";
std::cout << "link right_table=" << people.relationship("orders->customers")->right_table << "\n";
```

### Output

```text
people has(people)=true has(customers)=false
link right_table=people
```

The relationship is still named `"orders->customers"` (names are fixed at
declaration) but its `right_table` field now reads `"people"`.

Example 2. Renaming onto an existing name, and renaming to the same name,
are both refused.

```cpp
print_error("rename onto existing", [&] { (void)linked.rename("customers", "orders"); });
print_error("rename to same name", [&] { (void)linked.rename("customers", "customers"); });
```

### Output

```text
rename onto existing: Locus::rename: target name already exists 'orders'
rename to same name: Locus::rename: source and target names must differ
```

## See Also

- [`with`](with.md): replacing a catalog's data under its existing name.
- [`link`](link.md): the relationships a rename updates in place.
