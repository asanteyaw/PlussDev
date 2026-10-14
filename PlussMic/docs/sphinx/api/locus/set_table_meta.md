<!-- pluss-hand-authored -->

# Locus::set_table_meta, Locus::set_table_meta_

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::set_table_meta(std::string_view name, const TableMeta& value) const -> Locus
Locus& Locus::set_table_meta_(std::string_view name, const TableMeta& value)
```

## What It Does

Declares or replaces one catalog's `TableMeta` -- the explicitly-named
spelling of [`meta`](meta.md)'s two-argument (setter) overload; both call
the exact same implementation. `set_table_meta_` is the in-place form.

## Parameters

| Parameter | Meaning |
|---|---|
| `name` | The catalog to describe. |
| `value` | The complete replacement `TableMeta`. |

## Returns

`set_table_meta` returns a new `Locus`. `set_table_meta_` returns a
reference to the mutated receiver.

## Errors

Throws on an unknown catalog name.

## Examples

Example 1. `set_table_meta` and the setter overload of `meta` produce the
identical result.

```cpp
TableMeta meta_value;
meta_value.primary_key = {"id"};
const auto linked = make_linked_shop();
const auto described = linked.meta("customers", meta_value);
std::cout << "set_table_meta == meta: "
          << described.set_table_meta("customers", meta_value).has_table_meta("customers") << "\n";
```

### Output

```text
set_table_meta == meta: true
```

## See Also

- [`meta`](meta.md): the same setter, under its shorter alias, plus the
  getter overload and the in-place `meta_` form.
- [`has_table_meta`](has_table_meta.md): reading the declared metadata back.
