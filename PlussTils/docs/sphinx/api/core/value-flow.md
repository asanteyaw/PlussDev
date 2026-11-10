# Value Flow: flow and let

[Back to Core Utilities](../core.md) · [Core Utilities API Reference](../../api_reference.md)

Value flow has two utilities, and they answer different questions. `flow`
wraps any value in a box and lets you chain steps on it, with each step
returning a new box. `let` binds one of the core payload types and never lets
the name point at another value. The two do not depend on each other.

- [`flow` and `FlowBox`](../core-flow.md): the box, its value and
  unwrap accessors, `then` and `transform`, `tap`, `into`, and `as`.
- [`let`](../core-let.md): the binding, its read-only access, `map`, `peek`,
  `unwrap`, the payload trait, and the opt-in for new payload types.

For a guide with worked programs, see
[Values in pipelines: flow and let](../../manual/tils-suppose-let.md).

For named checks, derived Pluss values, and a failure trace for one input,
see [`suppose`](../core-suppose.md).
