<!-- pluss-hand-authored -->

# Thread::shares_payload_with

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::shares_payload_with(const Thread& other) const noexcept -> bool
```

## What It Does

Reports whether two `Thread` objects point at the *exact same* underlying payload
storage — a pointer-identity check on the internal reference-counted handle, not a
value-equality check. `Thread` uses copy-on-write: copying a thread (via the copy
constructor or `operator=`) is cheap and initially shares storage with the original;
the first mutating operation on either copy clones the storage before writing, so the
two threads stop sharing from that point on. Every purely functional method (the vast
majority of `Thread`'s API, which returns a brand-new `Thread` rather than mutating in
place) never causes sharing between its input and its output — sharing only ever
comes from copying the same handle.

```{note} This is not eq() or ==
Two threads holding identical values but built independently (e.g. two separate
`Thread(pluss::make_series({1.0, 2.0}))` calls) are **not** sharing payload — they're
distinct storage that happens to hold equal data. Use [`eq()`](eq.md) to compare
values.
```

## Parameters

**other** (`const Thread&`)
    The thread to compare storage identity against.

## Returns

`bool`
    `true` if `*this` and `other` currently point at the same backing storage;
    `false` otherwise.

## Examples

Example 1. A freshly copied thread shares payload with its source.

```cpp
using namespace pluss::table;

auto original = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto copy = original;
std::cout << std::boolalpha << original.shares_payload_with(copy);
```

### Output

```text
true
```

Example 2. Two threads built independently from equal-looking data never share,
even though they'd compare equal with [`eq()`](eq.md).

```cpp
auto a = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto b = Thread(pluss::make_series({10.0, 20.0, 30.0}));
std::cout << std::boolalpha << a.shares_payload_with(b);
```

### Output

```text
false
```

Example 3. A thread never shares payload with itself in the trivial sense of being
compared post-mutation — mutating a copy detaches it from the original (copy-on-write).

```cpp
auto base = Thread(pluss::make_series({1.0, 2.0, 3.0}));
auto shared_copy = base;
std::cout << std::boolalpha << base.shares_payload_with(shared_copy) << "\n";

shared_copy.fillnull_(0.0);
std::cout << std::boolalpha << base.shares_payload_with(shared_copy);
```

### Output

```text
true
false
```

Before the mutating `fillnull_()` call, `base` and `shared_copy` still point at the
same storage. Calling the mutating method on `shared_copy` clones its storage first
(copy-on-write), so afterward the two threads no longer share anything — and `base`
itself is left completely untouched by the mutation.

## Notes

- Pointer-identity on internal storage, not value equality — see the note above.
- `noexcept`: never throws, even comparing against an undefined thread.
- Copying a `Thread` (copy constructor or copy-assignment) is what creates sharing;
  every purely functional transform method returns freshly built storage instead.
- The first mutating (`_`-suffixed) call on either a shared thread or its copy
  detaches that one from the shared storage; the other side is unaffected.

## See Also

- [`Thread::defined`](defined.md): whether a thread has *any* backing storage at
  all, independent of sharing.
- [`Thread::eq`](eq.md): value equality, as opposed to this method's storage
  identity.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shape.html">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="shift.html">
    <span class="label">Next</span>
    <span class="title">shift</span>
  </a>
</div>
