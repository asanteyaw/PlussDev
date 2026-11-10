<!-- pluss-hand-authored -->

# Locus::meta, Locus::meta_

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::meta(std::string_view name) const -> std::optional<TableMeta>
Locus::meta(std::string_view name, const TableMeta& value) const -> Locus
Locus& Locus::meta_(std::string_view name, const TableMeta& value)
```

## What It Does

`meta(name)` (one argument) is an alias for [`table_meta`](has_table_meta.md)
-- the getter. `meta(name, value)` (two arguments) is an alias for
[`set_table_meta`](set_table_meta.md) -- the setter, which declares or
replaces `name`'s metadata and returns a new `Locus`. `meta_` is the
in-place form of the setter.

Declared metadata follows the catalog it describes: [`rename`](rename.md)
carries it to the new name, and [`drop`](drop.md)/[`without`](without.md)
remove it along with the catalog. It also reaches
[`nexus()`](policy.md)'s aligned `Nexus` view, so metadata declared on a
`Locus` is visible to DataFrame-oriented code too.

### `TableMeta` fields

| Field | Type | Default |
|---|---|---|
| `grain` | `std::string` | empty |
| `primary_key` | `std::vector<std::string>` | empty |
| `time_field` | `std::string` | empty |

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | The catalog to describe. |
| `value` | The complete replacement `TableMeta`, for the setter. |

## Returns

The getter returns `std::optional<TableMeta>`. The setter returns a new
`Locus` (`meta`) or a reference to the mutated receiver (`meta_`).

## Errors

The setter throws on an unknown catalog name.

## Examples

Example 1. Metadata follows a rename, and is dropped with its catalog.

```cpp
TableMeta meta;
meta.grain = "one row per customer";
meta.primary_key = {"id"};
const auto linked = make_linked_shop();
const auto described = linked.meta("customers", meta);
std::cout << "renamed still has meta=" << described.rename("customers", "people").has_table_meta("people") << "\n";
std::cout << "dropped loses meta=" << described.drop({"customers"}).has_table_meta("customers") << "\n";
```

### Output

```text
renamed still has meta=true
dropped loses meta=false
```

Example 2. Declaring metadata on an unknown catalog is refused; `meta_`
mutates in place.

```cpp
print_error("meta on unknown table", [&] { (void)linked.meta("ghost", meta); });
auto meta_inplace = linked;
meta_inplace.meta_("customers", meta);
std::cout << "meta_ changed in place=" << meta_inplace.has_table_meta("customers") << "\n";
```

### Output

```text
meta on unknown table: Locus::set_table_meta: unknown catalog 'ghost'
meta_ changed in place=true
```

## See Also

- [`has_table_meta`](has_table_meta.md), [`table_meta`](has_table_meta.md):
  the same getter under its explicit name.
- [`set_table_meta`](set_table_meta.md): the same setter under its explicit
  name.
