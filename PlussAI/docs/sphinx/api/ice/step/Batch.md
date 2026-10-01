# `pluss::ai::ice::Batch`

The flat, named-field batch [`Model::forward`](../model/Model.md)/
[`compute_loss`](../model/Model/compute_loss.md) operate on -- the
training loop's own `Batch`, distinct from
[`data::Batch`](../data/Batch.md) (see the `data::Batch` page's own note
for how the two relate; [`Source::transfer_batch`](../source/Source.md)
is the conversion point between them).

## Signature

```cpp
pluss::ai::ice::Batch::Batch() = default;
```

## Construction

Default-constructed, empty -- then built up field by field via
[`set`](Batch/set.md)/[`set_inputs`](Batch/set_inputs.md)/
[`set_targets`](Batch/set_targets.md). In practice, most callers never
construct one by hand -- a [`Source`](../source/Source.md) produces one
per mini-batch internally, and a [`Model`](../model/Model.md) subclass's
overridden step methods just read it.

## State

`inputs` / `targets` / every other named field
: `targets` is stored separately from the rest (`has_targets`/`targets`/
  `set_targets`); every other field -- `inputs` plus whatever else a
  caller has `set`, such as `static_inputs`/`encoder_inputs`/
  `decoder_inputs`/`encoder_target` for the `models_ts` catalog's own
  step overrides -- lives in the plain `fields()` list, read back by
  name through [`has`](Batch/has.md)/[`field`](Batch/field.md). Setting
  a name that already exists replaces that field's value in place
  rather than appending a duplicate.

## Methods

| Method | Returns | Covered on |
| --- | --- | --- |
| `has(name)` | `bool` | [`has`](Batch/has.md) |
| `field(name)` | `const Series&` / `Series&` | [`field`](Batch/field.md) |
| `fields()` | `const vector<BatchField>&` | [`fields`](Batch/fields.md) |
| `set(name, values)` | `void` | [`set`](Batch/set.md) |
| `has_inputs()` | `bool` | [`has_inputs`](Batch/has_inputs.md) |
| `inputs()` | `const Series&` / `Series&` | [`inputs`](Batch/inputs.md) |
| `set_inputs(values)` | `void` | [`set_inputs`](Batch/set_inputs.md) |
| `has_targets()` | `bool` | [`has_targets`](Batch/has_targets.md) |
| `targets()` | `const Series&` / `Series&` | [`targets`](Batch/targets.md) |
| `set_targets(values)` | `void` | [`set_targets`](Batch/set_targets.md) |

## Changelog

- **0.1** — the named-field batch `Model`'s step methods operate on:
  `inputs`/`targets` convenience accessors plus an open-ended named
  field list for models whose input isn't a single tensor.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Batch/Introduction
Batch/field
Batch/fields
Batch/has
Batch/has_inputs
Batch/has_targets
Batch/inputs
Batch/set
Batch/set_inputs
Batch/set_targets
Batch/targets
```
<!-- apiref:toctree:end -->
