<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6e706ad08539 -->

# Slot::push_

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
Slot& Slot::push_(const Value& value)
```

## What It Does

Appends `value` to the slot in place and returns `*this` for chaining.

## Parameters

**value** (`const Slot::Value&`)
    The value to append.

## Returns

`Slot&`
    A reference to the modified slot (`*this`).

## Examples

Example 1. Append in place; the same handle grows.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)});
slot.push_(pluss::make_series(99));
std::cout << "size=" << slot.size() << " last=" << std::get<pluss::Series>(slot.at(3)).host<std::int64_t>()[0];
```

### Output

```text
size=4 last=99
```

Example 2. Copy-on-write: mutating a copy leaves the original untouched.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)});
auto copy = slot;
copy.push_(pluss::make_series(99));
std::cout << "slot.size=" << slot.size() << " copy.size=" << copy.size();
```

### Output

```text
slot.size=2 copy.size=3
```

## Notes

- Mutates the slot in place — see [`push()`](push.md) for the
  non-mutating variant that returns a new `Slot`.
- `Slot` is copy-on-write: if the handle isn't the sole owner of its
  storage (see [`unique()`](unique.md)), `push_()` transparently clones the
  underlying values before appending, so other handles that were sharing
  the same storage are never affected by an in-place mutation.
- Requires the slot to be defined — an undefined `Slot` throws
  `"Slot: undefined handle"`.

## See Also

- [`Slot::push`](push.md): the non-mutating variant.
- [`Slot::unique`](unique.md): the reference-count check `push_()` uses
  internally to decide whether to clone before mutating.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="push.html">
    <span class="label">Previous</span>
    <span class="title">push</span>
  </a>
  <a class="next-link" href="shape.html">
    <span class="label">Next</span>
    <span class="title">shape</span>
  </a>
</div>
