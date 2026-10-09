# Text To Vectors

The runtime is built around one invariant: a batch has a layout, and every stage that reads it agrees on that
layout. The indexer chooses batch-first or sequence-first, the embedding follows the batch, and the encoders and
pooling functions check it. The result is that a batch can be handed from stage to stage without any stage
re-deriving its shape.

Padding is the other invariant. Short sequences are padded to the batch's longest, and the batch carries the
masks and lengths that let each stage ignore the padding. Nothing downstream should ever read a padded entry as
data.

Vocabulary is fixed at learning time. A model trained on one vocabulary must be used with the indexer that built
it, and the saved state of the indexer and the embedding table together reproduces the whole front end.

## See Also

- [`runtime`](../runtime.md) -- the family overview.
- [`SequenceBatch`](SequenceBatch.md) -- the value every stage passes along.

## Changelog

- **0.1** -- initial documented surface.
