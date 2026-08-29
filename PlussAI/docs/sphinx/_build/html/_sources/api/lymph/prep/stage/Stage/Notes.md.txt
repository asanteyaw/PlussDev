# `lymph::prep::Stage` Notes

## Descriptor Notes

Do not confuse a stage factory with a standalone transformer object.

These are different surfaces:

- `prep::stage::standard_scaler(...)` returns a `Stage`
- `prep::StandardScaler(...)` returns a reusable fitted object

They can share option types while still serving different roles.

## Pairing Notes

`Stage` becomes meaningful when paired with a `Route`. On its own, it says what
kind of transform exists, not which features it should touch.

## Related Pages

- [Introduction](Introduction.md)
- [contract](contract.md)
