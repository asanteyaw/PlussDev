# Lymph `models`

`models` is the Lymph main family for trainable modeling artifacts.

This family owns the objects users carry when they are deciding what to train,
what to compare, and what to keep.

The important separation is:

- `models` owns trainable artifacts and candidate-model workspaces
- `workflow` owns deployable orchestration around those artifacts
- `selection` owns comparison, validation, and search policy

If a user asks "what object is the thing I am fitting?", they are in
`models`.

## Subfamilies

```{toctree}
:maxdepth: 2

models/Model
builder <models/builder>
```

## Reading Order

1. [Model](models/Model.md)
2. [builder Introduction](models/builder/Introduction.md)
3. [Builder](models/builder/Builder.md)
