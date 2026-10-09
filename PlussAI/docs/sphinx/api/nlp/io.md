# `pluss::ai::nlp` IO

The io family saves a [`Workflow`](workflow/Workflow.md) to a stream and reads it back. The saved form records the
pipeline's language, its stages in order, and each stage's learned state, so a trained pipeline can be stored and
restored without retraining. The format is a plain text file with a schema version and an integrity checksum.

## See Also

- [`Introduction`](io/Introduction.md) -- what a saved workflow contains and how it is checked.
- [`write_workflow`](io/write_workflow.md) and [`read_workflow`](io/read_workflow.md) -- the two functions.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

io/Introduction
io/write_workflow
io/read_workflow
```
<!-- apiref:toctree:end -->
