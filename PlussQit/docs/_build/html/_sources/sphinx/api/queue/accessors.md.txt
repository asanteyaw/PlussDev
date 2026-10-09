<!-- pluss-hand-authored -->

# Queue::series, Queue::note, Queue::timeline, Queue::duration

[Back to Queue API Reference](../../queue_api_reference)

## Signature

```cpp
const Series& series() const;
Series& series();
const Note& note() const;
Note& note();
const Timeline& timeline() const;
Timeline& timeline();
const Duration& duration() const;
Duration& duration();
```

## What It Does

Each accessor returns a reference to the payload, typed as its real type,
*if* the `Queue` actually holds that kind. Calling the wrong one -- `note()`
on a `Queue` holding a `Series`, for instance -- throws, with a message
naming both the method you called and the kind the `Queue` actually holds.
The non-`const` overloads give a mutable reference, so calling a payload's
own in-place (`_`-suffixed) methods through the accessor mutates the value
the `Queue` holds.

These are the "I already know the kind, or I'm willing to find out by
exception" accessors. [`is<T>()`/`as<T>()`](generic-access.md) are the
template-based equivalent, and [`visit`](visit.md) avoids the kind check
entirely by handling every kind in one call.

This is also how most `Queue` work actually gets done: `Queue` itself only
has a handful of methods, but `series()`/`note()`/`timeline()`/`duration()`
hand back the real, full `Series`/`Note`/`Timeline`/`Duration` object, with
its entire own method surface -- not a restricted or `Queue`-specific view
of it. Chaining straight off the accessor, as in `q.series().mean()` or
`q.note().upper()`, is the ordinary way to call any method those four types
have; [`invoke`](invoke.md) is the alternative for when the method name
itself is only known at runtime.

## Parameters

None.

## Returns

A reference to the held payload, typed as `Series`, `Note`, `Timeline`, or
`Duration` according to which accessor was called.

## Errors

Throws (an exception derived from `std::exception`) when the `Queue`'s
actual `kind()` does not match the accessor called. The
message names the accessor and the actual kind, for example
`"Queue.as_note(): payload kind is Series"`.

## Examples

Example 1. Calling the matching accessor succeeds; calling a mismatched one
throws with a message naming the actual kind.

```cpp
pluss::Queue q(pluss::make_series({4.0, 5.0}));
std::cout << "q.series().numel(): " << q.series().numel() << '\n';
try {
  (void)q.note();
  std::cout << "q.note(): did not throw\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << "q.note() threw: " << msg.substr(0, msg.find('\n')) << '\n';
}
```

### Output

```text
q.series().numel(): 2
q.note() threw: Queue.as_note(): payload kind is Series
```

Example 2. The non-`const` overload gives a mutable reference, so the
payload's own in-place methods mutate the value the `Queue` holds.

```cpp
pluss::Queue mutable_q(pluss::make_series({10.0}));
mutable_q.series().add_(pluss::make_series({1.0}));
std::cout << "mutated through non-const series(): " << mutable_q.series().item<double>() << '\n';
```

### Output

```text
mutated through non-const series(): 11
```

`add_` is `Series`'s own in-place addition. Calling it through
`mutable_q.series()` changes the value stored inside `mutable_q` itself,
not a detached copy. This is the *only* way to mutate a `Queue`'s held
value -- [`invoke`](invoke.md) cannot reach an in-place method by name;
see its Example 5.

Example 3. A couple of ordinary, non-mutating method calls chained directly
off each accessor, one pair per kind. This is a small sample, not the whole
surface -- see each type's own reference for the rest.

```cpp
pluss::Queue series_q(pluss::make_series({1.0, 2.0, 3.0, 4.0}));
std::cout << "series().mean(): " << series_q.series().mean().item<double>() << '\n';
std::cout << "series().sum(): " << series_q.series().sum().item<double>() << '\n';

pluss::Queue note_q(pluss::make_note({"  Ab  "}));
std::cout << "note().strip(): " << note_q.note().strip() << '\n';
std::cout << "note().upper(): " << note_q.note().upper() << '\n';

pluss::Queue timeline_q(pluss::timeline::make("1999-04-17T21:15:23Z"));
std::cout << "timeline().year(): " << pluss::series::print(timeline_q.timeline().year()) << '\n';
std::cout << "timeline().add_days(1): " << pluss::timeline::print(timeline_q.timeline().add_days(1)) << '\n';

pluss::Queue duration_q(pluss::duration::hours(-5));
std::cout << "duration().abs(): " << pluss::duration::print(duration_q.duration().abs()) << '\n';
std::cout << "duration().total().hours(): " << pluss::series::print(duration_q.duration().total().hours()) << '\n';
```

### Output

```text
series().mean(): 2.5
series().sum(): 10
note().strip(): Note(shape=[1], dim=1, numel=1, encoding=utf8)
["Ab"]
note().upper(): Note(shape=[1], dim=1, numel=1, encoding=utf8)
["  AB  "]
timeline().year(): Series(shape=[], strides=[], dtype=int32, platform=cpu, numel=1, contiguous=true)
data=1999
timeline().add_days(1): Timeline(shape=[], tz=UTC)
1999-04-18 21:15:23
duration().abs(): Duration(shape=[], dim=0, numel=1)
0 days 05:00:00.000000000
duration().total().hours(): Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-5.0000
```

`strip()` and `upper()` are both non-mutating -- each returns a new `Note`
rather than changing the one `note_q` holds, which is why `note().upper()`
still shows the original, unstripped `"  Ab  "` rather than building on
`strip()`'s result. The same is true of `abs()`: `duration_q` itself is
untouched, so the later `duration().total().hours()` call still reports
`-5`, the original value, not the absolute one. Chaining off an accessor
calls that method the same way it would be called directly on the type --
it does not change what the accessor returns next time, unless the method
is one of the in-place, `_`-suffixed ones, as in Example 2.

## See Also

- [`kind`](kind.md): check the kind before calling the matching accessor,
  to avoid the exception path entirely.
- [`generic-access`](generic-access.md): `as<T>()`, which throws
  `std::bad_variant_access` instead of a named message on a kind mismatch.
- [Series API Reference](../../series_api_reference.md),
  [Note API Reference](../../note_api_reference.md),
  [Timeline API Reference](../../timeline_api_reference.md), and
  [Duration API Reference](../../duration_api_reference.md): the full method
  surface each accessor hands back.
