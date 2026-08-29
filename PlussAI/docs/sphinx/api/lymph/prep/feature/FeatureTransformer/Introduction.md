# `lymph::prep::FeatureTransformer` Introduction

## What This Type Represents

`FeatureTransformer` is the fitted object form of branch-aware feature
composition.

It is the thing you use when the preprocessing plan itself has structure:

- branch A gets scaling
- branch B gets one-hot encoding
- everything else is passed through or dropped

That full contract is learned once and then reused later as one object.

## Related Pages

- [learn](learn.md)
- [learn_map](learn_map.md)
- [map](map.md)
- [Notes](Notes.md)
