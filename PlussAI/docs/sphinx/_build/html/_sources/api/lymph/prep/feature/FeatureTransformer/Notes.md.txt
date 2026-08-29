# `lymph::prep::FeatureTransformer` Notes

## Why This Example Prints A Full `Series`

This is one of the few pages where printing the full mapped `Series` is
justified. The object’s main lesson is feature-space expansion and matrix
shape, not only a scalar metric or a one-column prediction table.

## Ownership Notes

This type is the object version of a whole routed feature contract. If your
project only needs one standalone scaler or encoder, this object is often more
machinery than you need.

## Related Pages

- [Introduction](Introduction.md)
- [learn_map](learn_map.md)
- [trained](trained.md)
