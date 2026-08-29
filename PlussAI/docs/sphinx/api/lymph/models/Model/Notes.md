# `lymph::models::Model` Notes

## State Notes

Three quick state readings explain most surprises on this surface:

- `trained()`
- `stage_count()`
- `task_kind()`

The most common source of confusion is pending materialization. A model can
have a real preparation plan attached while `stage_count()` still reads `0`.
That is not a contradiction. It means the plan has been staged but not yet
resolved against a concrete frame.

## Ownership Notes

Remember the split:

- `learn_` changes the left-hand object
- `learn` returns a trained copy

That split is useful in experiments where one template should be reused several
times without accidental mutation.

## Persistence Notes

`save` and `load` belong to the artifact layer. If the thing you need to
persist is broader than one model, step upward into `workflow` or `io`.

## Related Pages

- [Introduction](Introduction.md)
- [learn](learn.md)
- [save](save.md)
- [load](load.md)
