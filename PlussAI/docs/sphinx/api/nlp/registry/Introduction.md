# Names, Factories, And Restoration

A workflow saved to disk records each stage by its registry name, not by its code. Loading the workflow
asks the registry to create a default stage for each name, and then restores that stage's options and learned
state on top of the default. That two-step design is why every stage must be constructible with no arguments,
and why its state lines are complete enough to rebuild it.

Custom stages follow the same route. Register a factory under the stage's name once, at startup, and any saved
workflow that uses that name will load. A name the registry does not know is an error at load time, so a
missing registration is caught immediately rather than producing a half-built pipeline.

## See Also

- [`add`](StageRegistry/add.md) -- registers a factory.
- [`create`](StageRegistry/create.md) -- builds a stage from a name.

## Changelog

- **0.1** -- initial documented surface.
