<!-- pluss-hand-authored -->

# Queue::kind, Queue::Kind, Queue::kind_name, is_series, is_note, is_timeline, is_duration

[Back to Queue API Reference](../../queue_api_reference)

## Signature

```cpp
enum class Queue::Kind : std::uint8_t { Series, Note, Timeline, Duration };

Kind kind() const noexcept;
std::string_view kind_name() const noexcept;
static constexpr std::string_view kind_name(Kind k) noexcept;

bool is_series() const noexcept;
bool is_note() const noexcept;
bool is_timeline() const noexcept;
bool is_duration() const noexcept;
```

## What It Does

`kind()` reports which of the four payload types a `Queue` is actually
holding, as a `Queue::Kind` enum value. `kind_name()` gives the same answer
as a human-readable string -- `"Series"`, `"Note"`, `"Timeline"`, or
`"Duration"` -- and the static overload converts an arbitrary `Kind` value
to its name without needing a `Queue` instance at all. `is_series()`,
`is_note()`, `is_timeline()`, and `is_duration()` are convenience
shorthands for `kind() == Queue::Kind::X`, one per kind, none of them mutually
exclusive with the others being `false` -- exactly one is ever `true` for a
given `Queue`.

None of these methods can throw, and none of them require knowing the
payload type in advance -- they are the methods you call first, before
deciding how to handle a `Queue` you were just handed.

## Parameters

**k** (`Kind`, static overload only)

The kind value to name.

## Returns

`kind()` returns a `Queue::Kind`. `kind_name()` (both overloads) returns a
`std::string_view` into a function-local string literal, valid for the
program's lifetime. The four `is_*` methods return `bool`.

## Examples

Example 1. `kind()`, `kind_name()`, and the static `kind_name(Kind)`
overload, read directly off a `Series`-holding `Queue`.

```cpp
pluss::Queue q(pluss::make_series({1.0, 2.0}));
std::cout << "kind() == Queue::Kind::Series: " << (q.kind() == pluss::Queue::Kind::Series) << '\n';
std::cout << "kind_name(): " << q.kind_name() << '\n';
std::cout << "static kind_name(Kind::Note): " << pluss::Queue::kind_name(pluss::Queue::Kind::Note) << '\n';
std::cout << "static kind_name(Kind::Timeline): " << pluss::Queue::kind_name(pluss::Queue::Kind::Timeline) << '\n';
std::cout << "static kind_name(Kind::Duration): " << pluss::Queue::kind_name(pluss::Queue::Kind::Duration) << '\n';
```

### Output

```text
kind() == Queue::Kind::Series: true
kind_name(): Series
static kind_name(Kind::Note): Note
static kind_name(Kind::Timeline): Timeline
static kind_name(Kind::Duration): Duration
```

The static overload names any `Kind` value directly -- useful for printing
a kind you got from somewhere other than a live `Queue`, such as a stored
enum value.

Example 2. `is_series()`/`is_note()`/`is_timeline()`/`is_duration()` on a
`Series`-holding and a `Note`-holding `Queue`. Exactly one is `true` in each
case.

```cpp
pluss::Queue q(pluss::make_series({1.0, 2.0}));
pluss::Queue note_q(pluss::make_note({"x"}));
std::cout << "series.is_series(): " << q.is_series() << '\n';
std::cout << "series.is_note(): " << q.is_note() << '\n';
std::cout << "note.is_note(): " << note_q.is_note() << '\n';
std::cout << "note.is_series(): " << note_q.is_series() << '\n';
std::cout << "note.is_timeline(): " << note_q.is_timeline() << '\n';
std::cout << "note.is_duration(): " << note_q.is_duration() << '\n';
```

### Output

```text
series.is_series(): true
series.is_note(): false
note.is_note(): true
note.is_series(): false
note.is_timeline(): false
note.is_duration(): false
```

## See Also

- [`accessors`](accessors.md): typed access once you know the kind.
- [`generic-access`](generic-access.md): `is<T>()`, the template-based
  equivalent of the four `is_*` methods.
- [`visit`](visit.md): act on whichever payload is held without branching
  on `kind()` yourself.
