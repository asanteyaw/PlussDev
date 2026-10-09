# What A Saved Workflow Contains

A saved workflow is a list of stage blocks. Each block names the stage, so the loader can rebuild it through the
[stage registry](../registry.md), and then carries the stage's state lines, so the stage is restored exactly as it was
saved. The header records the language and the number of stages, and a checksum covers the stage payload.

The checksum is what makes a damaged file fail early. A file cut short or altered in the middle does not parse into
a half-built pipeline; the loader rejects it with a diagnostic about the mismatch. Files written by earlier versions
of the format still load, and are upgraded as they are read, so a saved pipeline does not need to be regenerated when
the library is updated.

## See Also

- [`write_workflow`](write_workflow.md) -- saves.
- [`read_workflow`](read_workflow.md) -- restores.

## Changelog

- **0.1** -- initial documented surface.
