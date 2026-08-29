# `lymph::prep::StandardScaler` Introduction

## What This Type Represents

`StandardScaler` is the object form of learned standardization.

It owns the fitted state needed to transform later frames consistently:

- learned centers
- learned scales
- learned feature identity

That is the difference between this type and a one-off numeric formula in user
code. The state is explicit, reusable, and inspectable.

## Related Pages

- [features](features.md)
- [learn](learn.md)
- [learn_map](learn_map.md)
- [map](map.md)
- [summary](summary.md)
- [Notes](Notes.md)
