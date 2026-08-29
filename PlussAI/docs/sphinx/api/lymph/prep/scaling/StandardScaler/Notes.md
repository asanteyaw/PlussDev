# `lymph::prep::StandardScaler` Notes

## Standalone-vs-Stage Notes

Use `prep::StandardScaler` when the scaler itself should be a reusable object.
Use `prep::stage::standard_scaler(...)` when scaling should only exist as one
step inside a broader prep graph.

## Reuse Notes

The object becomes most valuable after learning once and mapping many times.
That is the main reason to keep scaling in an object instead of recomputing it
inline.

## Related Pages

- [Introduction](Introduction.md)
- [learn_map](learn_map.md)
- [summary](summary.md)
