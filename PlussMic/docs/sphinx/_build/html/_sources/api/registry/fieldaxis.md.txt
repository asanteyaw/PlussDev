<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c775f1dc90ed -->

# Registry::fieldaxis

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
RegistryFieldAxisAccessor Registry::fieldaxis() const
```

## What It Does

The entry point into **field-name and field-kind filtering** — `fieldaxis()` binds
the current registry and hands back a `RegistryFieldAxisAccessor`, documented in
full on its own pages (see
[`RegistryFieldAxisAccessor` API Reference](../../registry_fieldaxis_accessor_api_reference)):
name-pattern filters (`like()`, `match()`, `prefix()`, `suffix()`) and value-kind
filters (`kind()`, `series()`, `note()`, `timeline()`, `timeindex()`, `mixed()`,
`nulls()`, `empty()`). Each accessor method returns a new `Registry` containing only
the matching fields.

## Parameters

`None`

## Returns

`RegistryFieldAxisAccessor`
    A binder for the current registry — call one of its methods to select a subset
    of fields by name or by value kind.

## Examples

Example 1. Select only the fields whose values are entirely `Series`-kind.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

auto series_only = rg.fieldaxis().series();
for (const auto& key : series_only.fields()) std::cout << key << " ";
```

### Output

```text
id
```

## Notes

- `fieldaxis()` alone does no work — it only binds the registry; call one of its
  methods to actually filter fields.
- Distinct from [`fields(keys)`](fields.md), which selects fields you already know
  the names of — `fieldaxis()` selects fields by pattern or by value kind instead.
- The full accessor surface is documented separately — see
  [`RegistryFieldAxisAccessor` API Reference](../../registry_fieldaxis_accessor_api_reference).

## See Also

- [`Registry::fields`](fields.md): field access when you already know the exact
  field names, rather than a pattern or kind — `fields(keys)` is the lower-level
  explicit-list projection this accessor's filters are built on top of.
- [`AtlasLabelAccessor`](../../atlas_label_accessor_api_reference): a similarly-shaped
  filtering accessor on the table model, for depth labels rather than field names.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="end.html">
    <span class="label">Previous</span>
    <span class="title">end</span>
  </a>
  <a class="next-link" href="fields.html">
    <span class="label">Next</span>
    <span class="title">fields</span>
  </a>
</div>
