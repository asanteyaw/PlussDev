<!-- pluss-hand-authored -->

# Queue::visit

[Back to Queue API Reference](../../queue_api_reference)

## Signature

```cpp
template <class F>
decltype(auto) visit(F&& f) const;

template <class F>
decltype(auto) visit(F&& f);
```

## What It Does

Calls `f` with the held payload, typed as its real type, with no kind check
or accessor call needed up front -- `visit` is `std::visit` over the
`Queue`'s internal variant. `f` is typically a generic lambda (`[](const
auto& payload) { ... }`) that uses `if constexpr` to branch on the payload's
actual type, since `f` must be callable with all four payload types even if
it only does something meaningful for one or two of them.

This is the method to reach for instead of checking `kind()` and calling
the matching accessor by hand, when the logic genuinely differs per kind
and a single generic function can express all four branches together.

## Parameters

**f** (callable)

A callable invocable with `const Series&`, `const Note&`, `const Timeline&`,
and `const Duration&` (or their non-`const` equivalents, for the non-`const`
overload) -- in practice almost always a generic lambda.

## Returns

Whatever `f` returns for the branch that was actually taken, forwarded
through `decltype(auto)`.

## Examples

Example 1. A generic lambda that returns a different string depending on
whether the payload is a `Series`.

```cpp
pluss::Queue q(pluss::make_series({1.0, 2.0, 3.0}));
const auto label = q.visit([](const auto& payload) -> std::string {
  using T = std::decay_t<decltype(payload)>;
  if constexpr (std::is_same_v<T, pluss::Series>) {
    return "a series with " + std::to_string(payload.numel()) + " elements";
  } else {
    return "something else";
  }
});
std::cout << "visit result: " << label << '\n';
```

### Output

```text
visit result: a series with 3 elements
```

The lambda's `if constexpr` branch means the `else` arm never actually
calls `.numel()` on a `Note`, `Timeline`, or `Duration` -- it is only
compiled for the branches where it is well-formed, the same way any
`if constexpr` over a generic lambda parameter works.

## See Also

- [`kind`](kind.md): the lower-level, non-generic way to branch on kind.
- [`invoke`](invoke.md): dispatch to the payload's own named methods,
  rather than running arbitrary C++ against it.
