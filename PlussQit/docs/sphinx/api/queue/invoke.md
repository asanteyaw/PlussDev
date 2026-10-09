<!-- pluss-hand-authored -->

# Queue::invoke

[Back to Queue API Reference](../../queue_api_reference)

## Signature

```cpp
Queue invoke(std::string_view method, std::span<const Arg> args = {}) const;
Queue invoke(std::string_view method, std::initializer_list<Arg> args) const;
Queue invoke(std::initializer_list<std::string_view> methods) const;
```

## What It Does

`invoke` calls a method by name on the held payload -- `Series`, `Note`,
`Timeline`, or `Duration` -- the same way you would call it directly on
that type, but through a name string instead of a member-function call.
This is the general-purpose way to reach a method generically, without
first branching on `kind()` and calling the matching accessor.

`Timeline` also accepts a handful of friendlier names --
`"year"`, `"month"`, `"day"`, `"hour"`, `"minute"`, `"second"`,
`"quarter"`, `"weekday"`, `"dayofyear"`, `"week"` -- which are translated to
`Timeline`'s own registered method names before the call (so `"year"`
reaches the same method `Timeline::year()` calls directly). Any other
method name passes through unchanged. If a `Timeline` method isn't found
under the (possibly translated) name, two fallbacks are tried: a
`Series`-flavored lookup, then a `Note`-flavored one, since some `Timeline`
operations are implemented alongside those types. `Duration` looks a method
up directly, with the same two fallbacks on a miss.

Each argument converts implicitly -- an integer, a floating-point number, a
boolean, a string, a list of integers, or a `Series`/`Note`/`Timeline`/
`Duration` value can all be written directly in the argument list (`{2.0}`,
`{"some text"}`) with no explicit wrapping needed, the same way `Queue`'s
own constructors are implicit.

Three overloads exist: a single method with a list of arguments (spelled
either as a `span` or a braced `initializer_list` -- both accept `{2.0}`
directly), and a third overload that takes a list of *method names* rather
than arguments, applying each one in turn, with no arguments to any of
them, to the result of the last.

## Parameters

**method** (`std::string_view`)

The method name to call, in the payload's own naming convention (or one of
the handful of `Timeline` aliases above).

**args** (a list of arguments, default empty)

The arguments to pass. Each one converts implicitly from an integer,
floating-point number, boolean, string, list of integers, or a
`Series`/`Note`/`Timeline`/`Duration` value.

**methods** (a list of method names, three-argument-less overload only)

A sequence of method names, each applied with no arguments to the result
of the previous call, left to right. An empty method name anywhere in the
list is refused.

## Returns

A new `Queue` wrapping whatever the called method returned, re-wrapped by
kind the same way the original constructors are.

## Errors

Throws when the payload doesn't have a method by that name (after any
`Timeline` alias translation and fallback attempts), or when an argument of
a type the method doesn't accept is passed. The chained,
method-names-only overload also throws if any name in the list is empty.

`invoke` also cannot reach an in-place, trailing-underscore method
(`Series`'s `add_`, `mul_`, and the rest) by name -- it always throws,
regardless of the payload's kind. Mutating a `Queue`'s held value is only
possible through the non-`const` [accessor](accessors.md), calling the
in-place method directly: `q.series().add_(x)`, not
`q.invoke("add_", {x})`.

## Examples

Example 1. `invoke("mul", ...)` on a `Series`-holding `Queue` -- the
argument converts implicitly, with no explicit wrapping.

```cpp
pluss::Queue series_q(pluss::make_series({1.0, 2.0, 3.0}));
pluss::Queue doubled = series_q.invoke("mul", {2.0});
std::cout << "series invoke(mul, 2.0): " << pluss::series::print(doubled.series()) << '\n';
```

### Output

```text
series invoke(mul, 2.0): Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.0000, 4.0000, 6.0000]
```

Example 2. `invoke("upper")` on a `Note`-holding `Queue`.

```cpp
pluss::Queue note_q(pluss::make_note({"ab", "cd"}));
pluss::Queue note_upper = note_q.invoke("upper");
std::cout << "note invoke(upper): " << note_upper.note() << '\n';
```

### Output

```text
note invoke(upper): Note(shape=[2], dim=1, numel=2, encoding=utf8)
["AB", "CD"]
```

Example 3. `invoke("year")` on a `Timeline`-holding `Queue` -- `"year"` is
translated to the name `Timeline::year()` is registered under before the
call.

```cpp
pluss::Queue timeline_q(pluss::timeline::make("1999-04-17T21:15:23Z"));
pluss::Queue year_q = timeline_q.invoke("year");
std::cout << "timeline invoke(year): "
          << pluss::series::print(year_q.series()) << '\n';
```

### Output

```text
timeline invoke(year): Series(shape=[], strides=[], dtype=int32, platform=cpu, numel=1, contiguous=true)
data=1999
```

The result comes back as a plain `Series` wrapped in a `Queue` --
`Timeline::year()` returns a `Series`, not another `Timeline`, so the
returned `Queue`'s `kind()` is `Series`, not `Timeline`.

Example 4. The chained, method-names-only overload: `neg` then `abs`,
applied in sequence to a `Series`-holding `Queue`, with no arguments to
either.

```cpp
pluss::Queue series_q(pluss::make_series({1.0, 2.0, 3.0}));
pluss::Queue chained = series_q.invoke({"neg", "abs"});
std::cout << "chained invoke({neg, abs}): " << pluss::series::print(chained.series()) << '\n';
```

### Output

```text
chained invoke({neg, abs}): Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 2.0000, 3.0000]
```

Negating then taking the absolute value returns the original magnitudes --
`neg` followed by `abs` is a round trip for any already-positive input.

Example 5. `invoke` cannot reach an in-place method -- `"add_"` throws,
even though `Series::add_` itself is a perfectly real method.

```cpp
pluss::Queue series_q(pluss::make_series({1.0, 2.0, 3.0}));
try {
  pluss::Queue bad = series_q.invoke("add_", {1.0});
  std::cout << "invoke(add_): did not throw\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << "invoke(add_) threw: " << msg.substr(0, msg.find('\n')) << '\n';
}
```

### Output

```text
invoke(add_) threw: Unrouted op on CPU (strict mode): add_(double) has no route to a kernel; it would otherwise return the receiver unchanged (a silent identity). Implement it or remove the call.
```

To mutate the `Series` a `Queue` holds, call `add_` directly off the
non-`const` accessor instead -- `series_q.series().add_(x)` -- which is
real C++ method dispatch, not a name string, so it reaches `add_` without
going through `invoke` at all. See [`accessors`](accessors.md)'s Example 2.

## See Also

- [`operators`](operators.md): `+`, `-`, `*`, and the comparison operators
  are themselves thin wrappers around `invoke` for `Series`, `Timeline`, and
  `Duration` payloads.
- [`where`](where.md): the one operation `Queue` exposes as a static method
  rather than through `invoke`.
