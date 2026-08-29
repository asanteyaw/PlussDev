# `lymph::prep::Route::with_remainder`

## Signature

```cpp
lymph::prep::Route& lymph::prep::Route::with_remainder(
    lymph::RouteRemainderPolicy policy) noexcept;
```

## What It Does

`with_remainder(...)` attaches remainder behavior to the route itself.

That matters because a route is not only "which features do I claim?" It can
also carry the policy for what should happen to everything not claimed by that
stage.

In practice, the usual question is whether the remainder should pass through or
be dropped at the point where the route is used.

## Related Pages

- [resolve](resolve.md)
- [Notes](Notes.md)
