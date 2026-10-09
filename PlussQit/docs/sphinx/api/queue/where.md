<!-- pluss-hand-authored -->

# Queue::where

[Back to Queue API Reference](../../queue_api_reference)

## Signature

```cpp
static Queue where(const Queue& condition, const Queue& x, const Queue& y);
```

## What It Does

A ternary conditional select: elementwise, picks `x` where `condition` is
`true` and `y` where it is `false`. `condition` must hold a `Series` (a
boolean mask); `x` and `y` must have the same kind as each other, which may
be `Series`, `Timeline`, or `Duration` -- `Note` is not supported.

For a `Series` result, this calls `condition.series().where(x, y)`
directly. For `Timeline` and `Duration`, it forwards to
`x.invoke("where", {condition, y})`, so the same method dispatch that backs
[`invoke`](invoke.md) handles the actual selection.

## Parameters

**condition** (`const Queue&`)

Must hold a `Series` of booleans. Any other kind is refused.

**x**, **y** (`const Queue&`)

The two choices, picked elementwise by `condition`. Must have the same
kind as each other; a mismatch is refused, and `Note` is refused
regardless of whether `x` and `y` agree.

## Returns

A `Queue` of the same kind as `x` and `y`.

## Errors

Throws when `condition` does not hold a `Series`, when `x` and `y` have
different kinds, or when `x`/`y` hold `Note` (ternary selection on text is
not supported).

## Examples

Example 1. Selecting between two `Series`-holding `Queue`s by a boolean
mask.

```cpp
pluss::Queue mask(pluss::make_series({true, false, true}));
pluss::Queue x(pluss::make_series({1.0, 2.0, 3.0}));
pluss::Queue y(pluss::make_series({10.0, 20.0, 30.0}));
pluss::Queue picked = pluss::Queue::where(mask, x, y);
std::cout << "where(mask, x, y): " << pluss::series::print(picked.series()) << '\n';
```

### Output

```text
where(mask, x, y): Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1.0000, 20.0000, 3.0000]
```

The mask `[true, false, true]` picks `x`'s first and third elements and
`y`'s second, matching `mask ? x : y` applied elementwise.

Example 2. `Note` is not supported, regardless of the mask.

```cpp
pluss::Queue mask(pluss::make_series({true, false, true}));
pluss::Queue note_a(pluss::make_note({"a"}));
pluss::Queue note_b(pluss::make_note({"b"}));
try {
  pluss::Queue bad = pluss::Queue::where(mask, note_a, note_b);
  std::cout << "where on Note: did not throw\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << "where on Note threw: " << msg.substr(0, msg.find('\n')) << '\n';
}
```

### Output

```text
where on Note threw: Queue::where failed: Queue::where: Note ternary selection is not supported
```

## See Also

- [`invoke`](invoke.md): the `Timeline`/`Duration` branches of `where`
  forward here under the hood.
- [`operators`](operators.md): the comparison operators that typically
  build the `condition` mask `where` consumes.
