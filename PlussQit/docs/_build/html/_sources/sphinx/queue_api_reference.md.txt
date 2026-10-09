<!-- pluss-hand-authored -->

# Queue API Reference

[Back to PlussQit API Reference](../api_reference.md)

This page documents `pluss::Queue` one method at a time, with complete
behavioral prose and executable-derived outputs.

## What `Queue` Is

`Queue` is a dynamic holder over PlussQit's four core value types --
[`Series`](series_api_reference.md), [`Note`](note_api_reference.md),
[`Timeline`](timeline_api_reference.md), and
[`Duration`](duration_api_reference.md) -- for code that works generically
across columns whose concrete type isn't known until runtime, such as a
table layer that stores Series, Note, Timeline, and Duration columns side by
side. A `Queue` always holds exactly one of the four; `kind()` reports which,
and `invoke("method", args...)` routes a MATLAB-style method name to a
method on whichever of `Series`, `Note`, `Timeline`, or `Duration` the
payload actually is.

`Queue` is not a fifth value type with its own arithmetic rules -- every
operator and every `invoke` call forwards to the held payload's own
behavior, so `Queue`'s contract is almost entirely "whatever the payload
would do," plus the up-front kind bookkeeping that makes the forwarding
possible without the caller needing a `switch` on type.

## `invoke()` vs. the accessors: which to use

There are two ways to call a method through a `Queue`, and they are not
interchangeable:

| | [`invoke("name", args)`](api/queue/invoke.md) | [`series()`/`note()`/`timeline()`/`duration()`](api/queue/accessors.md) |
|---|---|---|
| Method is picked | at run time, by a name string | at compile time, by a real C++ call |
| Works without knowing the kind | yes -- same call shape for any kind | no -- you call the accessor for the kind you expect, or it throws |
| Method surface reached | whatever that kind's dispatch exposes under that name | the type's *entire* public surface, exactly as its own reference documents it |
| Can mutate the `Queue`'s value | never -- always returns a new `Queue` | yes, through the non-`const` overload and an in-place (`_`-suffixed) method |
| Argument checking | at run time, against the resolved method | at compile time, by ordinary C++ overload resolution |

In short: reach for `series()`/`note()`/`timeline()`/`duration()` and call
the method directly -- `q.series().mean()`, `q.note().strip()` -- whenever
you already know (or are willing to assert) the kind, which is most code.
It is the only way to mutate, it gives you the type's real, full method
surface with normal compile-time checking, and [`invoke.md`'s Examples
1--4](api/queue/invoke.md) show the same operations are reachable either
way. Reach for `invoke("name", args)` instead only when the method name
itself has to be decided at run time -- generic code that doesn't branch on
`kind()`, a name read from configuration, or a single code path meant to
apply the same operation across whichever of the four kinds a `Queue`
happens to hold.

The two routes call the same underlying method, so they agree when both are
usable:

```cpp
pluss::Queue q(pluss::make_series({1.0, 2.0, 3.0, 4.0}));
const double via_invoke = q.invoke("mean").series().item<double>();
const double via_accessor = q.series().mean().item<double>();
std::cout << "via invoke(\"mean\"): " << via_invoke << '\n';
std::cout << "via series().mean(): " << via_accessor << '\n';
std::cout << "same result: " << (via_invoke == via_accessor) << '\n';
```

### Output

```text
via invoke("mean"): 2.5
via series().mean(): 2.5
same result: true
```

## Includes

```cpp
#include <pluss/pluss.h>
// or directly
#include <pluss/queue.h>
```

## Practical Construction Patterns

Example 1. Wrap each of the four payload types. `kind_name()` reports which
one a given `Queue` is actually holding.

```cpp
pluss::Queue from_series(pluss::make_series({1.0, 2.0, 3.0}));
pluss::Queue from_note(pluss::make_note({"a", "b"}));
pluss::Queue from_timeline(pluss::timeline::make("1999-04-17T21:15:23Z"));
pluss::Queue from_duration(pluss::duration::days(3));
std::cout << from_series.kind_name() << ' ' << from_note.kind_name() << ' '
          << from_timeline.kind_name() << ' ' << from_duration.kind_name() << '\n';
```

### Output

```text
Series Note Timeline Duration
```

See [`construction`](api/queue/construction.md) for every constructor form.

## Coverage

This reference covers the canonical public surface:

- construction: four constructors, one per payload type, each with a copy
  and a move overload -- [`construction`](api/queue/construction.md)
- identifying the payload: the `Kind` enum, [`kind`](api/queue/kind.md)
- typed access back to the payload: [`accessors`](api/queue/accessors.md)
  (`series()`, `note()`, `timeline()`, `duration()`)
- structural introspection: [`state`](api/queue/state.md) (`defined()`,
  `dim()`, `numel()`)
- generic templated access: [`generic-access`](api/queue/generic-access.md)
  (`is<T>()`, `as<T>()`)
- a visitor over whichever payload is held: [`visit`](api/queue/visit.md)
- calling a method on the held payload by name:
  [`invoke`](api/queue/invoke.md)
- every public operator call form: [`operators`](api/queue/operators.md)
- the static ternary selector: [`where`](api/queue/where.md)
- printing: [`stream`](api/queue/stream.md) (`operator<<`)

Each distinct method has its own page. `Queue::Arg` (the implicit-conversion
wrapper `invoke`'s argument list accepts) is documented inline on
[`invoke`](api/queue/invoke.md) rather than on its own page, since it has no
behavior of its own beyond holding one argument.

## Using This Reference

- Use the left sidebar to open one `Queue` method page at a time.
- Each page embeds verified output directly under the example that produced
  it, captured from `PlussQit/examples/CodeH/queue_docverify_demo.cpp`.

```{toctree}
:hidden:
:maxdepth: 1

api/queue/construction
api/queue/kind
api/queue/accessors
api/queue/state
api/queue/generic-access
api/queue/visit
api/queue/invoke
api/queue/operators
api/queue/where
api/queue/stream
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../api_reference.html">
    <span class="label">Previous</span>
    <span class="title">PlussQit API Reference</span>
  </a>
  <a class="next-link" href="api/queue/construction.html">
    <span class="label">Next</span>
    <span class="title">construction</span>
  </a>
</div>
