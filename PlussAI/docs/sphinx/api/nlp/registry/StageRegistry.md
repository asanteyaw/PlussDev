# `pluss::ai::nlp::registry::StageRegistry`

The process-wide catalog of stage factories, keyed by registry name. It is a singleton: every part of the
program sees the same catalog, and it starts with the eight built-in stages registered.

## Signature

```cpp
pluss::ai::nlp::registry::StageRegistry::StageRegistry();
```

The constructor is private. A caller reaches the catalog through [`instance`](StageRegistry/instance.md), and the
copy operations are deleted, so there is exactly one catalog.

## Construction

The catalog is created on first use. Its built-in entries are `rule_tokenizer`, `sentencizer`, `lemmatizer`,
`matcher`, `tagger`, `syntax`, `ner`, and `textcat`, each mapped to a factory that builds the stage in its default
configuration.

## State

The catalog holds one factory per name. A factory takes no arguments and returns a `Stage`. The catalog is guarded
for concurrent use: lookups share a lock, and registration takes it exclusively.

## Methods

Each method has its own page.

| Method | Returns | Covered on |
| --- | --- | --- |
| `instance()` | `StageRegistry&` | [`instance`](StageRegistry/instance.md) |
| `add(name, factory)` | `bool` | [`add`](StageRegistry/add.md) |
| `contains(name) const` | `bool` | [`contains`](StageRegistry/contains.md) |
| `create(name) const` | `Stage` | [`create`](StageRegistry/create.md) |
| `names() const` | `std::vector<std::string>` | [`names`](StageRegistry/names.md) |

## See Also

- [`Introduction`](StageRegistry/Introduction.md) -- how names drive restoration.
- [`IStage`](../core/IStage.md) -- the interface a registered stage implements.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

StageRegistry/Introduction
StageRegistry/add
StageRegistry/contains
StageRegistry/create
StageRegistry/instance
StageRegistry/names
```
<!-- apiref:toctree:end -->
