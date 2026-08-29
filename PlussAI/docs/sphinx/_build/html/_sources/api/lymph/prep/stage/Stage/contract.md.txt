# `lymph::prep::Stage::contract`

## Signature

```cpp
lymph::prep::TransformContract lymph::prep::Stage::contract() const noexcept;
```

## What It Does

`contract()` exposes behavioral metadata about the attached stage kind.

This is where the system can answer questions such as:

- does this transform change feature count?
- is it lossy?
- is it stateful?
- is an inverse theoretically available?

That information matters when you are auditing a prep graph rather than merely
running it.

## Related Pages

- [Notes](Notes.md)
- [../../route/Route.md](../../route/Route.md)
