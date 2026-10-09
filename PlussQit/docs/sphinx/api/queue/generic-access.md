<!-- pluss-hand-authored -->

# Queue::is, Queue::as

[Back to Queue API Reference](../../queue_api_reference)

## Signature

```cpp
template <class T>
bool is() const noexcept;

template <class T>
const T& as() const;

template <class T>
T& as();
```

## What It Does

`is<T>()` and `as<T>()` are the template-based equivalents of
[`is_series()`/`series()`](kind.md) and their three siblings, for generic
code written against a type parameter instead of a hardcoded accessor
name -- `T` is one of `Series`, `Note`, `Timeline`, or `Duration`. `is<T>()`
checks whether the held payload is exactly `T`; `as<T>()` returns a
reference to it.

The behavioral difference from the named accessors
([`series()`/`note()`/`timeline()`/`duration()`](accessors.md)) is in what
happens on a kind mismatch: the named accessors throw a named failure
naming both the method and the actual kind; `as<T>()` calls
`std::get<T>` directly on the internal `std::variant` with no such check, so
a mismatch throws a bare `std::bad_variant_access` instead, with no
`Queue`-specific message.

## Parameters

**T** (template parameter)

One of `Series`, `Note`, `pluss::codeh::Timeline`, or
`pluss::codeh::Duration`.

## Returns

`is<T>()` returns `bool`. `as<T>()` returns a reference to the payload,
typed as `T`.

## Errors

`as<T>()` throws `std::bad_variant_access` when `T` does not match the
`Queue`'s actual `kind()`. Unlike the named accessors, the exception
carries no message naming the actual kind -- check `is<T>()` or `kind()`
first if that information matters to the caller.

## Examples

Example 1. `is<T>()` on a `Note`-holding `Queue`, true for the matching
type and false for a mismatched one.

```cpp
pluss::Queue q(pluss::make_note({"p", "q"}));
std::cout << "is<pluss::Note>(): " << q.is<pluss::Note>() << '\n';
std::cout << "is<pluss::Series>(): " << q.is<pluss::Series>() << '\n';
std::cout << "as<pluss::Note>().numel(): " << q.as<pluss::Note>().numel() << '\n';
```

### Output

```text
is<pluss::Note>(): true
is<pluss::Series>(): false
as<pluss::Note>().numel(): 2
```

Example 2. Calling `as<T>()` with the wrong `T` throws
`std::bad_variant_access`, not a `Queue`-named message.

```cpp
pluss::Queue q(pluss::make_note({"p", "q"}));
try {
  (void)q.as<pluss::Series>();
  std::cout << "as<pluss::Series>(): did not throw\n";
} catch (const std::bad_variant_access& e) {
  std::cout << "as<pluss::Series>() threw std::bad_variant_access: " << e.what() << '\n';
}
```

### Output

```text
as<pluss::Series>() threw std::bad_variant_access: bad_variant_access
```

`std::bad_variant_access::what()` gives the generic library message
`"bad_variant_access"`, with no mention of `Queue`, `Series`, or `Note` --
check `is<T>()` first (as in Example 1) when the caller needs to know why
the access would fail, rather than catching the exception to find out.

## See Also

- [`accessors`](accessors.md): the named, non-templated equivalents, which
  throw a more informative message on a kind mismatch.
- [`kind`](kind.md): `is_series()`/`is_note()`/`is_timeline()`/
  `is_duration()`, the non-templated equivalent of `is<T>()`.
