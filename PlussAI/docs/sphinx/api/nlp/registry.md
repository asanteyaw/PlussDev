# `pluss::ai::nlp` Registry

The registry is the catalog that maps a stage's registry name to a factory that builds it. It is how a saved
workflow is restored: the file names each stage, and the registry turns each name back into a stage before the
stage's saved state is applied to it. The eight built-in stages are always present, and an application can add its
own stage types so that custom stages restore the same way.

The registry is a single process-wide object. Registering a name is idempotent in effect: a name that is already
present is not replaced, so two parts of an application cannot silently swap a stage out from under each other.

## See Also

- [`StageRegistry`](registry/StageRegistry.md) -- the catalog class and its methods.
- [`IStage`](core/IStage.md) -- the interface a registered stage implements.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

registry/Introduction
registry/StageRegistry
```
<!-- apiref:toctree:end -->
