<!-- pluss-hand-authored -->

# Locus::flow

[Back to Locus API Reference](../../locus_api_reference)

## Signature

```cpp
Locus::flow(const std::function<Locus(const Locus&)>& fn) const -> Locus
```

## What It Does

Threads this `Locus` through `fn` and returns whatever `fn` returns --
`locus.flow(f)` is exactly `f(locus)`, spelled so it reads as a step in a
fluent chain instead of wrapping the whole expression. It exists purely for
call-site readability: `shop.link(...).flow(my_custom_step).join(...)` keeps
a reader-defined transformation inline in a chain, the same shape as every
other `Locus`-returning method, rather than breaking the chain to name an
intermediate variable.

## Parameters

| Parameter | Meaning |
|---|---|
| `fn` | A callable from a `Locus` to a `Locus` -- typically a lambda capturing whatever the step needs. |

## Returns

Whatever `fn(*this)` returns.

## Examples

Example 1. A `flow` step dropping a catalog, inline in what reads as a
normal chain.

```cpp
const auto linked = make_linked_shop();
const auto flowed = linked.flow([](const Locus& l) { return l.drop({"items"}); });
std::cout << "flowed ntables=" << flowed.ntables() << "\n";
```

### Output

```text
flowed ntables=2
```

`flow([](const Locus& l) { return l.drop({"items"}); })` here is exactly
`linked.drop({"items"})` -- `flow` adds nothing operationally, only a place
to put an arbitrary step (a named function, a lambda built elsewhere) inline
in a chain.

## See Also

- [`apply_all`](apply_all.md): threading a transform through every catalog's
  data, rather than the whole `Locus`.
