<!-- pluss-hand-authored -->

# Queue construction

[Back to Queue API Reference](../../queue_api_reference)

## Signature

```cpp
Queue(const Series& v);
Queue(Series&& v);
Queue(const Note& v);
Queue(Note&& v);
Queue(const Timeline& v);
Queue(Timeline&& v);
Queue(const Duration& v);
Queue(Duration&& v);
```

## What It Does

Wraps one payload value -- a `Series`, `Note`, `Timeline`, or `Duration` --
in a `Queue`. There are eight constructors in total: one copying and one
moving overload for each of the four payload types. Every one of them is
implicit (none is marked `explicit`), so a `Queue` can be built directly
from a payload expression without naming the constructor, and a function
that takes a `const Queue&` accepts a bare `Series`/`Note`/`Timeline`/
`Duration` argument too.

Once built, a `Queue` is not reassignable to a different kind by any public
method -- the only way to change which payload a `Queue` holds is to
construct a new one.

## Parameters

**v** (`const Series&`, `Series&&`, `const Note&`, `Note&&`, `const
Timeline&`, `Timeline&&`, `const Duration&`, or `Duration&&`)

The payload to wrap. Which overload is selected fixes the new `Queue`'s
`kind()`.

## Returns

A `Queue` whose `kind()` matches the payload type that was passed in.

## Examples

Example 1. One `Queue` per payload type, each built from a different
factory.

```cpp
pluss::Queue from_series(pluss::make_series({1.0, 2.0, 3.0}));
pluss::Queue from_note(pluss::make_note({"a", "b"}));
pluss::Queue from_timeline(pluss::timeline::make("1999-04-17T21:15:23Z"));
pluss::Queue from_duration(pluss::duration::days(3));
std::cout << "from_series.kind_name()=" << from_series.kind_name() << '\n';
std::cout << "from_note.kind_name()=" << from_note.kind_name() << '\n';
std::cout << "from_timeline.kind_name()=" << from_timeline.kind_name() << '\n';
std::cout << "from_duration.kind_name()=" << from_duration.kind_name() << '\n';
```

### Output

```text
from_series.kind_name()=Series
from_note.kind_name()=Note
from_timeline.kind_name()=Timeline
from_duration.kind_name()=Duration
```

Each `Queue` reports the kind of the value it was built from, regardless of
which of the four factories produced that value.

## Notes

- The constructors are implicit by design: a function parameter typed
  `const Queue&` accepts a plain `Series`, `Note`, `Timeline`, or `Duration`
  directly, with no explicit wrapping at the call site.
- `Queue` has no default constructor. There is no "empty" or "undefined"
  `Queue` -- every `Queue` holds exactly one of the four payloads from the
  moment it is constructed.

## See Also

- [`kind`](kind.md): identifying which payload a `Queue` holds.
- [`accessors`](accessors.md): getting the payload back out.
